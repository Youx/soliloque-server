/*
 * soliloque-server, an open source implementation of the TeamSpeak protocol.
 * Copyright (C) 2009 Hugo Camboulive <hugo.camboulive AT gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "control_packet.h"
#include "log.h"
#include "database.h"
#include "packet_tools.h"
#include "acknowledge_packet.h"
#include "server_stat.h"
#include "channel.h"
#include "player.h"

#include <errno.h>
#include <string.h>
#include <strings.h>

/**
 * Send a "player switched channel" notification to all players.
 *
 * @param s the server
 * @param pl the player who switched
 * @param from the channel the player was in
 * @param to the channel he is moving to
 */
static void s_notify_switch_channel(struct player *pl, struct channel *from, struct channel *to)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 38;
	struct server *s = pl->in_chan->in_server;
	struct player_channel_privilege *new_priv;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_switch_channel, packet allocation failed : %s.", strerror(errno));
		return;
	}
	new_priv = get_player_channel_privilege(pl, to);
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_SWITCHCHAN, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(pl->public_id, &ptr);		/* ID of player who switched */
	wu32(from->id, &ptr);			/* ID of previous channel */
	wu32(to->id, &ptr);			/* channel the player switched to */
	wu16(new_priv->flags, &ptr);

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a request from a client to switch to another channel.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the sender
 * @param cli_len the size of cli_addr
 */
void *c_req_switch_channel(char *data, unsigned int len, struct player *pl)
{
	struct channel *to, *from;
	uint32_t to_id;
	char *pass, *ptr;
	struct server *s = pl->in_chan->in_server;

	ptr = data + 24;
	to_id = ru32(&ptr);
	to = get_channel_by_id(s, to_id);
	pass = rstaticstring(29, &ptr);

	if (to != NULL) {
		send_acknowledge(pl);		/* ACK */
		/* the player can join if one of these :
		 * - there is no password on the channel
		 * - he has the privilege to join without giving a password
		 * - he gives the correct password */
		if (!(ch_getflags(to) & CHANNEL_FLAG_PASSWORD)
				|| player_has_privilege(pl, SP_CHA_JOIN_WO_PASS, to)
				|| strcmp(pass, to->password) == 0) {
			logger(LOG_INFO, "Player switching to channel %s.", to->name);
			from = pl->in_chan;
			if (move_player(pl, to)) {
				s_notify_switch_channel(pl, from, to);
				logger(LOG_INFO, "Player moved, notify sent.");
				/* TODO change privileges */
			}
		}
	}
	free(pass);
	return NULL;
}

/**
 * Notify all players of a player's status change
 * has been granted/revoked.
 *
 * @param pl the player who granted/revoked the privilege
 * @param tgt the player whose privileges are going to change
 * @param right the offset of the right (1 << right == CHANNEL_PRIV_XXX)
 * @param on_off switch this right on or off
 */
static void s_notify_player_attr_changed(struct player *pl, uint16_t new_attr)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 30;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_attr_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_PL_STATUS, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(pl->public_id, &ptr);		/* ID of player whose attr changed */
	wu16(new_attr, &ptr);			/* new attributes */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Notify all players that a player's channel privilege
 * has been granted/revoked.
 *
 * @param pl the player who granted/revoked the privilege
 * @param tgt the player whose privileges are going to change
 * @param right the offset of the right (1 << right == CHANNEL_PRIV_XXX)
 * @param on_off switch this right on or off
 */
static void s_notify_player_ch_priv_changed(struct player *pl, struct player *tgt, char right, char on_off)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 34;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_ch_priv_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_PL_CHPRIV, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(tgt->public_id, &ptr);		/* ID of player whose channel priv changed */
	wu8(on_off, &ptr);			/* switch the priv ON/OFF */
	wu8(right, &ptr);			/* offset of the privilege (1<<right) */
	wu32(pl->public_id, &ptr);		/* ID of the player who changed the priv */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a request to change a player's channel privileges
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_change_player_ch_priv(char *data, unsigned int len, struct player *pl)
{
	struct player *tgt;
	uint32_t tgt_id;
	char on_off, right, *ptr;
	int priv_required;

	send_acknowledge(pl);		/* ACK */

	ptr = data + 24;
	tgt_id = ru32(&ptr);
	on_off = ru8(&ptr);
	right = ru8(&ptr);
	tgt = get_player_by_public_id(pl->in_chan->in_server, tgt_id);

	switch (1 << right) {
	case CHANNEL_PRIV_CHANADMIN:
		priv_required = (on_off == 0) ? SP_PL_GRANT_CA : SP_PL_REVOKE_CA;
		break;
	case CHANNEL_PRIV_OP:
		priv_required = (on_off == 0) ? SP_PL_GRANT_OP : SP_PL_REVOKE_OP;
		break;
	case CHANNEL_PRIV_VOICE:
		priv_required = (on_off == 0) ? SP_PL_GRANT_VOICE : SP_PL_REVOKE_VOICE;
		break;
	case CHANNEL_PRIV_AUTOOP:
		priv_required = (on_off == 0) ? SP_PL_GRANT_AUTOOP : SP_PL_REVOKE_AUTOOP;
		break;
	case CHANNEL_PRIV_AUTOVOICE:
		priv_required = (on_off == 0) ? SP_PL_GRANT_AUTOVOICE : SP_PL_REVOKE_AUTOVOICE;
		break;
	default:
		return NULL;
	}
	if (tgt != NULL && player_has_privilege(pl, priv_required, tgt->in_chan)) {
		logger(LOG_INFO, "Player priv before : 0x%x", player_get_channel_privileges(tgt, tgt->in_chan));
		if (on_off == 2)
			player_clr_channel_privilege(tgt, tgt->in_chan, (1 << right));
		else if(on_off == 0) {
			player_set_channel_privilege(tgt, tgt->in_chan, (1 << right));
			/* if the player was requesting a voice and we granted him, remove the voice request */
			if (priv_required == SP_PL_GRANT_VOICE && tgt->player_attributes & PL_ATTR_REQUEST_VOICE) {
				tgt->player_attributes &= ~PL_ATTR_REQUEST_VOICE;
				s_notify_player_attr_changed(tgt, tgt->player_attributes);
			}
		}
		logger(LOG_INFO, "Player priv after  : 0x%x", player_get_channel_privileges(tgt, tgt->in_chan));
		s_notify_player_ch_priv_changed(pl, tgt, right, on_off);
	}
	return NULL;
}

/**
 * Notify all players that a player's global flags
 * has been granted/revoked.
 *
 * @param pl the player who granted/revoked the privilege
 * @param tgt the player whose privileges are going to change
 * @param right the offset of the right (1 << right == GLOBAL_FLAG_XXX)
 * @param on_off switch this right on or off (0 = add, 2 = remove)
 */
void s_notify_player_sv_right_changed(struct player *pl, struct player *tgt, char right, char on_off)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 34;
	struct server *s = tgt->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_sv_right_changed, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CHANGE_PL_SVPRIV, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(tgt->public_id, &ptr);		/* ID of player whose global flags changed */
	wu8(on_off, &ptr);			/* set or unset the flag */
	wu8(right, &ptr);			/* offset of the flag (1 << right) */
	if (pl != NULL) {
		wu32(pl->public_id, &ptr);/* ID of player who changed the flag */
	} else {
		wu32(0, &ptr);
	}

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a request to change a player's global flags
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_change_player_sv_right(char *data, unsigned int len, struct player *pl)
{
	struct player *tgt;
	uint32_t tgt_id;
	char on_off, right;
	int priv_required;
	struct channel *ch;
	struct player_channel_privilege *priv;
	size_t iter, iter2;
	char *ptr;

	send_acknowledge(pl);		/* ACK */

	ptr = data + 24;
	tgt_id = ru32(&ptr);
	on_off = ru8(&ptr);
	right = ru8(&ptr);;
	tgt = get_player_by_public_id(pl->in_chan->in_server, tgt_id);

	switch (1 << right) {
	case GLOBAL_FLAG_SERVERADMIN:
		priv_required = (on_off == 0) ? SP_PL_GRANT_SA : SP_PL_REVOKE_SA;
		break;
	case GLOBAL_FLAG_ALLOWREG:
		priv_required = (on_off == 0) ? SP_PL_GRANT_ALLOWREG : SP_PL_REVOKE_ALLOWREG;
		break;
	case GLOBAL_FLAG_REGISTERED:
		priv_required = (on_off == 0) ? SP_PL_ALLOW_SELF_REG : SP_PL_DEL_REGISTRATION;
		break;
	default:
		logger(LOG_WARN, "c_req_change_player_sv_right : not implemented for privilege : %i", 1<<right);
		return NULL;
	}
	if (tgt != NULL && player_has_privilege(pl, priv_required, tgt->in_chan)) {
		logger(LOG_INFO, "Player sv rights before : 0x%x", tgt->global_flags);
		if (on_off == 2) {
			tgt->global_flags &= (0xFF ^ (1 << right));
			/* special case : removing a registration */
			if (1 << right == GLOBAL_FLAG_REGISTERED) {
				db_del_registration(tgt->in_chan->in_server->conf, tgt->in_chan->in_server, tgt->reg);
				/* associate the player privileges to the player instead of the registration */
				ar_each(struct channel *, ch, iter, tgt->in_chan->in_server->chans)
					ar_each(struct player_channel_privilege *, priv, iter2, ch->pl_privileges)
						if (priv->reg == PL_CH_PRIV_REGISTERED && priv->pl_or_reg.reg == tgt->reg) {
							priv->reg = PL_CH_PRIV_UNREGISTERED;
							priv->pl_or_reg.pl = tgt;
						}
					ar_end_each;
				ar_end_each;
				free(tgt->reg);
				tgt->reg = NULL;
			}
		} else if(on_off == 0) {
			tgt->global_flags |= (1 << right);
		}

		/* special case : registration */
		logger(LOG_INFO, "Player sv rights after  : 0x%x", tgt->global_flags);
		s_notify_player_sv_right_changed(pl, tgt, right, on_off);
	}
	return NULL;
}

/**
 * Handle a request to change a player's global flags
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_change_player_attr(char *data, unsigned int len, struct player *pl)
{
	uint16_t attributes;
	char *ptr;

	send_acknowledge(pl);		/* ACK */

	ptr = data + 24;

	attributes = ru16(&ptr);
	logger(LOG_INFO, "Player sv rights before : 0x%x", pl->player_attributes);
	pl->player_attributes = attributes;
	logger(LOG_INFO, "Player sv rights after  : 0x%x", pl->player_attributes);
	s_notify_player_attr_changed(pl, attributes);
	return NULL;
}

static void s_notify_player_moved(struct player *tgt, struct player *pl, struct channel *from, struct channel *to)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 42;
	struct server *s = pl->in_chan->in_server;
	struct player_channel_privilege *new_priv;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_moved, packet allocation failed : %s.", strerror(errno));
		return;
	}
	new_priv = get_player_channel_privilege(tgt, to);
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_PLAYER_MOVED, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(tgt->public_id, &ptr);		/* ID of player who switched */
	wu32(from->id, &ptr);			/* ID of previous channel */
	wu32(to->id, &ptr);			/* channel the player switched to */
	wu32(pl->public_id, &ptr);
	wu16(new_priv->flags, &ptr);

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

void *c_req_move_player(char *data, unsigned int len, struct player *pl)
{
	struct channel *to, *from;
	uint32_t to_id;
	uint32_t tgt_id;
	struct player *tgt;
	struct server *s = pl->in_chan->in_server;
	char *ptr;

	ptr = data + 24;
	tgt_id = ru32(&ptr);
	tgt = get_player_by_public_id(s, tgt_id);
	to_id = ru32(&ptr);
	to = get_channel_by_id(s, to_id);

	if (to != NULL && pl != NULL) {
		send_acknowledge(pl);		/* ACK */
		/* check privilege */
		if (player_has_privilege(pl, SP_ADM_MOVE_PLAYER, to)) {
			logger(LOG_INFO, "Player moving another one to channel %s.", to->name);
			from = tgt->in_chan;
			if (move_player(tgt, to)) {
				s_notify_player_moved(tgt, pl, from, to);
				logger(LOG_INFO, "Player moved, notify sent.");
			}
		}
	}
	return NULL;
}

static void s_resp_player_muted(struct player *by, struct player *tgt, uint8_t on_off)
{
	char *data, *ptr;
	size_t data_size = 29;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_moved, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_PLAYER_MUTED_UNMUTED, &ptr);
	wu32(by->private_id, &ptr);		/* private ID */
	wu32(by->public_id, &ptr);		/* public ID */
	wu32(by->f0_s_counter, &ptr);		/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(tgt->public_id, &ptr);		/* ID of player who was muted */
	wu8(on_off, &ptr);			ptr += 1;

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	packet_add_crc_d(data, data_size);

	send_to(by->in_chan->in_server, data, data_size, 0, by);
	by->f0_s_counter++;

	free(data);
}

void *c_req_mute_player(char *data, unsigned int len, struct player *pl)
{
	uint32_t tgt_id;
	uint8_t on_off;	/* 1 = MUTE, 0 = UNMUTE */
	struct player *tgt;
	struct server *s = pl->in_chan->in_server;
	char *ptr = data + 24;

	tgt_id = ru32(&ptr);
	tgt = get_player_by_public_id(s, tgt_id);
	on_off = ru8(&ptr);

	send_acknowledge(pl);
	if (pl == tgt) {
		logger(LOG_WARN, "player tried to mute himself, that should not happen!");
		return NULL;
	}
	if (tgt == NULL) {
		logger(LOG_WARN, "player tried to unmute a player that does not exist, that should not happen!");
		return NULL;
	}

	if (on_off == 1) {
		/* MUTE */
		if (!ar_has(pl->muted, tgt)) {
			ar_insert(pl->muted, tgt);
			s_resp_player_muted(pl, tgt, on_off);
		} else {
			logger(LOG_WARN, "player tried to mute a player he already muted!");
		}
	} else if (on_off == 0) {
		/* UNMUTE */
		if (ar_has(pl->muted, tgt)) {
			ar_remove(pl->muted, tgt);
			s_resp_player_muted(pl, tgt, on_off);
		} else {
			logger(LOG_WARN, "player tried to unmute a player he did not mute!");
		}
	} else {
		logger(LOG_WARN, "c_req_mute_player : on_off != 0/1");
	}

	return NULL;
}

static void s_notify_player_requested_voice(struct player *pl)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 58;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_requested_voice, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_VOICE_REQUESTED, &ptr);
	ptr += 4;				/* private ID */
	ptr += 4;				/* public ID */
	ptr += 4;				/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	wu32(pl->public_id, &ptr);		/* player who requested voice */
	wstaticstring(pl->voice_request, 29, &ptr);	/* length of reason */

	assert(ptr - data == data_size);
	
	ar_each(struct player *, tmp_pl, iter, s->players)
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

void *c_req_request_voice(char *data, unsigned int len, struct player *pl)
{
	char *vr, *ptr;
	/*if (len != 54) {
		TODO
	}*/
	send_acknowledge(pl);		/* ACK */
	/* if the channel is not moderated or the player already has voice, refuse! */
	if (!(player_get_channel_privileges(pl, pl->in_chan) & CHANNEL_PRIV_VOICE) ||
		!(pl->in_chan->flags & CHANNEL_FLAG_MODERATED)) {
		logger(LOG_WARN, "c_req_request_voice : player is already V or channel is not M.");
		return NULL;
	}
	bzero(pl->voice_request, 30);
	ptr = data + 24;
	vr = rstaticstring(29, &ptr);
	strcpy(pl->voice_request, vr);
	pl->player_attributes |= PL_ATTR_REQUEST_VOICE;

	s_notify_player_requested_voice(pl);
	free(vr);
	return NULL;
}
