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
#include "packet_tools.h"
#include "server_stat.h"
#include "acknowledge_packet.h"

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

/**
 * Send a "player kicked" notification to all players.
 *
 * @param s the server
 * @param kicker the player who kicked
 * @param kicked the player kicked from the server
 * @param reason the reason the player was kicked
 */
static void s_notify_kick_server(struct player *kicker, struct player *kicked, char *reason)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	struct server *s = kicker->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_kick_server, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERLEFT;	ptr += 2;	/* */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = kicked->public_id;	ptr += 4;	/* ID of player who left */
	*(uint16_t *)ptr = 2;			ptr += 2;	/* visible notification : kicked */
	*(uint32_t *)ptr = kicker->public_id;	ptr += 4;	/* kicker ID */
	*(uint8_t *)ptr = MIN(29, strlen(reason));	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, *(ptr - 1));	ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, iter, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handles a server kick request.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_kick_server(char *data, unsigned int len, struct player *pl)
{
	uint32_t target_id;
	char *reason;
	struct player *target;
	struct server *s = pl->in_chan->in_server;

	if (len != 60) {
		logger(LOG_WARN, "c_req_kick_server, packet has invalid size : %i instead of %i.", len, 60);
		return NULL;
	}

	memcpy(&target_id, data + 24, 4);
	target = get_player_by_public_id(s, target_id);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(player_has_privilege(pl, SP_OTHER_SV_KICK, target->in_chan)) {
			reason = strndup(data + 29, MIN(29, data[28]));
			logger(LOG_INFO, "Reason for kicking player %s : %s", target->name, reason);
			s_notify_kick_server(pl, target, reason);
			remove_player(s, pl);
			free(reason);
		}
	}

	return NULL;
}

/**
 * Send a "player kicked from channel" notification to all players.
 *
 * @param s the server
 * @param kicker the player who kicked
 * @param kicked the player kicked from the server
 * @param reason the reason the player was kicked
 * @param kicked_to the channel the player is moved to
 */
static void s_notify_kick_channel(struct player *kicker, struct player *kicked, 
		char *reason, struct channel *kicked_from)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 68;
	struct server *s = kicker->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_kick_channel, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHKICK_PL;		ptr += 2;	/* */
	/* private ID */				ptr += 4;	/* filled later */
	/* public ID */					ptr += 4;	/* filled later */
	/* packet counter */				ptr += 4;	/* filled later */
	/* packet version */				ptr += 4;	/* not done yet */
	/* empty checksum */				ptr += 4;	/* filled later */
	*(uint32_t *)ptr = kicked->public_id;		ptr += 4;	/* ID of player who was kicked */
	*(uint32_t *)ptr = kicked_from->id;		ptr += 4;	/* channel the player was kicked from */
	*(uint32_t *)ptr = kicker->public_id;		ptr += 4;	/* ID of the kicker */
	*(uint16_t *)ptr = 0;				ptr += 2;	/* visible notification : kicked */
	*(uint8_t *)ptr = MIN(29,strlen(reason));	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, *(ptr - 1));		ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, iter, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handles a channel kick request.
 */
void *c_req_kick_channel(char *data, unsigned int len, struct player *pl)
{
	uint32_t target_id;
	char *reason;
	struct player *target;
	struct channel *def_chan;
	struct server *s = pl->in_chan->in_server;

	if (len != 60) {
		logger(LOG_WARN, "c_req_kick_channel, packet has invalid size : %i instead of %i.", len, 60);
		return NULL;
	}
	memcpy(&target_id, data + 24, 4);
	target = get_player_by_public_id(s, target_id);
	def_chan = get_default_channel(s);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if (player_has_privilege(pl, SP_OTHER_CH_KICK, target->in_chan)) {
			reason = strndup(data + 29, MIN(29, data[28]));
			logger(LOG_INFO, "Reason for kicking player %s : %s", target->name, reason);
			s_notify_kick_channel(pl, target, reason, pl->in_chan);
			move_player(pl, def_chan);
			/* TODO update player channel privileges etc... */

			free(reason);
		}
	}

	return NULL;
}

/**
 * Send a notification to all players that target has been banned.
 *
 * @param s the server
 * @param pl the player who gave the order to ban
 * @param target the player who is banned
 * @param duration the duration of the ban (0 = unlimited)
 * @param reason the reason of the ban
 */
static void s_notify_ban(struct player *pl, struct player *target, uint16_t duration, char *reason)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_ban, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERLEFT;	ptr += 2;	/* */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = target->public_id;	ptr += 4;	/* ID of player banned */
	*(uint16_t *)ptr = 2;			ptr += 2;	/* kick */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* banner ID */
	*(uint8_t *)ptr = MIN(29,strlen(reason));	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, *(ptr - 1));	ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, iter, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, pl);
			tmp_pl->f0_s_counter++;
	ar_end_each;
	free(data);
}

/**
 * Handle a player ban request.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_ban(char *data, unsigned int len, struct player *pl)
{
	uint32_t ban_id;
	struct player *target;
	char *reason;
	uint16_t duration;
	struct server *s = pl->in_chan->in_server;

	memcpy(&ban_id, data + 24, 4);
	memcpy(&duration, data + 28, 2);

	target = get_player_by_public_id(s, ban_id);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(player_has_privilege(pl, SP_ADM_BAN_IP, target->in_chan)) {
			reason = strndup(data + 29, MIN(29, data[28]));
			add_ban(s, new_ban(0, target->cli_addr->sin_addr, reason));
			logger(LOG_INFO, "Reason for banning player %s : %s", target->name, reason);
			s_notify_ban(pl, target, duration, reason);
			remove_player(s, target);
			free(reason);
		}
	}
	return NULL;
}


/**
 * Send the list of bans to a player
 *
 * @param s the server
 * @param pl the player who asked for the list of bans
 */
static void s_resp_bans(struct player *pl)
{
	char *data, *ptr;
	int data_size, tmp_size;
	struct ban *b;
	struct server *s = pl->in_chan->in_server;
	size_t iter;
	
	data_size = 24;
	data_size += 4;	/* number of bans */

	ar_each(struct ban *, b, iter, s->bans)
		data_size += ban_to_data_size(b);
	ar_end_each;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_ban, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_BANLIST;		ptr += 2;	/* */
	*(uint32_t *)ptr = pl->private_id;	ptr += 4;	/* private ID */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* public ID */
	*(uint32_t *)ptr = pl->f0_s_counter;	ptr += 4;	/* packet counter */
	/* packet version */			ptr += 4;	/* filled later */
	/* checksum */				ptr += 4;	/* filled at the end */
	*(uint16_t *)ptr = s->bans->used_slots;	ptr += 4;	/* number of bans */
	logger(LOG_INFO, "number of bans : %zu", s->bans->used_slots);
	ar_each(struct ban *, b, iter, s->bans)
		tmp_size = ban_to_data(b, ptr);
		ptr += tmp_size;
	ar_end_each;
	
	packet_add_crc_d(data, data_size);
	logger(LOG_INFO, "list of bans : sending %i bytes", data_size);
	send_to(s, data, data_size, 0, pl);

	pl->f0_s_counter++;
	free(data);
}

/**
 * Handle a player request for the list of bans*
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_list_bans(char *data, unsigned int len, struct player *pl)
{
	send_acknowledge(pl);		/* ACK */
	s_resp_bans(pl);
	return NULL;
}

/**
 * Handles a request to remove a ban.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_remove_ban(char *data, unsigned int len, struct player *pl)
{
	struct in_addr ip;
	struct ban *b;
	struct server *s = pl->in_chan->in_server;

	if(player_has_privilege(pl, SP_ADM_BAN_IP, NULL)) {
		send_acknowledge(pl);		/* ACK */
		inet_aton(data+24, &ip);
		b = get_ban_by_ip(s, ip);
		if (b != NULL)
			remove_ban(s, b);
	}
	return NULL;
}

/**
 * Handles a request to add an IP ban.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_ip_ban(char *data, unsigned int len, struct player *pl)
{
	struct in_addr ip;
	uint16_t duration;
	struct server *s = pl->in_chan->in_server;

	if(player_has_privilege(pl, SP_ADM_BAN_IP, NULL)) {
		send_acknowledge(pl);		/* ACK */
		duration = *(uint16_t *)(data + 24);
		inet_aton(data+26, &ip);
		add_ban(s, new_ban(duration, ip, "IP BAN"));
	}
	return NULL;
}

