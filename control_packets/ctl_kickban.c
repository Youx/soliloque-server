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
		ERROR("s_notify_kick_server, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_PLAYERLEFT, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(kicked->public_id, &ptr);	/* ID of player who left */
	wu16(2, &ptr);	/* visible notification : kicked */
	wu32(kicker->public_id, &ptr);	/* kicker ID */
	wstaticstring(reason, 29, &ptr);

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
	char *reason, *ptr;
	struct player *target;
	struct server *s = pl->in_chan->in_server;

	if (len != 60) {
		WARNING("c_req_kick_server, packet has invalid size : %i instead of %i.", len, 60);
		return NULL;
	}

	ptr = data + 24;
	target_id = ru32(&ptr);
	target = get_player_by_public_id(s, target_id);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(player_has_privilege(pl, SP_OTHER_SV_KICK, target->in_chan)) {
			ptr = data + 28;
			reason = rstaticstring(29, &ptr);
			INFO("Reason for kicking player %s : %s", target->name, reason);
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
		ERROR("s_notify_kick_channel, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_CHKICK_PL, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(kicked->public_id, &ptr);	/* ID of player who was kicked */
	wu32(kicked_from->id, &ptr);	/* channel the player was kicked from */
	wu32(kicker->public_id, &ptr);	/* ID of the kicker */
	wu16(0, &ptr);	/* visible notification : kicked */
	wstaticstring(reason, 29, &ptr);/* reason message */

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
 * Handles a channel kick request.
 */
void *c_req_kick_channel(char *data, unsigned int len, struct player *pl)
{
	uint32_t target_id;
	char *reason, *ptr;
	struct player *target;
	struct channel *def_chan;
	struct server *s = pl->in_chan->in_server;

	if (len != 60) {
		WARNING("c_req_kick_channel, packet has invalid size : %i instead of %i.", len, 60);
		return NULL;
	}
	ptr = data + 24;
	target_id = ru32(&ptr);
	target = get_player_by_public_id(s, target_id);
	def_chan = get_default_channel(s);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if (player_has_privilege(pl, SP_OTHER_CH_KICK, target->in_chan)) {
			ptr = data + 28;
			reason = rstaticstring(29, &ptr);
			INFO("Reason for kicking player %s : %s", target->name, reason);
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
		ERROR("s_notify_ban, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_PLAYERLEFT, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(target->public_id, &ptr);	/* ID of player banned */
	wu16(2, &ptr);	/* kick */
	wu32(pl->public_id, &ptr);	/* banner ID */
	wstaticstring(reason, 29, &ptr);/* reason message */

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
	char *reason, *ptr;
	uint16_t duration;
	struct server *s = pl->in_chan->in_server;

	if (len != 30) {
		WARNING("c_req_ban, packet has invalid size : %i instead of %i.", len, 30);
		return NULL;
	}
	ptr = data + 24;
	ban_id = ru32(&ptr);
	duration = ru16(&ptr);

	target = get_player_by_public_id(s, ban_id);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(player_has_privilege(pl, SP_ADM_BAN_IP, target->in_chan)) {
			reason = rstaticstring(29, &ptr);
			add_ban(s, new_ban(0, target->cli_addr->sin_addr, reason));
			INFO("Reason for banning player %s : %s", target->name, reason);
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
		ERROR("s_resp_ban, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_BANLIST, &ptr);	/* */
	wu32(pl->private_id, &ptr);	/* private ID */
	wu32(pl->public_id, &ptr);	/* public ID */
	wu32(pl->f0_s_counter, &ptr);	/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* checksum */
	wu32(s->bans->used_slots, &ptr);/* number of bans */
	INFO("number of bans : %zu", s->bans->used_slots);
	ar_each(struct ban *, b, iter, s->bans)
		tmp_size = ban_to_data(b, ptr);
		ptr += tmp_size;
	ar_end_each;
	
	packet_add_crc_d(data, data_size);
	INFO("list of bans : sending %i bytes", data_size);
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
	if (len != 24) {
		WARNING("c_req_list_bans, packet has invalid size : %i instead of %i.", len, 24);
		return NULL;
	}
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

	/* ipv4 adress is at least 7 characters + \0 */
	if (len < 32) {
		WARNING("c_req_remove_bans, packet has invalid size (<%i)", 24);
		return NULL;
	}
	if (data[len - 1] != '\0') {
		WARNING("IP adress was not NULL terminated");
		return NULL;
	}
	if(player_has_privilege(pl, SP_ADM_BAN_IP, NULL)) {
		send_acknowledge(pl);		/* ACK */
		inet_aton(data + 24, &ip);
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
	char *ptr;

	if (len < 34) {
		WARNING("c_req_ip_ban, packet has invalid size (<%i)", 34);
		return NULL;
	}
	if (data[len - 1] != '\0') {
		WARNING("IP adress was not NULL terminated");
		return NULL;
	}
	if(player_has_privilege(pl, SP_ADM_BAN_IP, NULL)) {
		send_acknowledge(pl);		/* ACK */
		ptr = data + 24;
		duration = ru16(&ptr);
		inet_aton(ptr, &ip);
		add_ban(s, new_ban(duration, ip, "IP BAN"));
	}
	return NULL;
}

