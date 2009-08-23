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
#include "player.h"
#include "server.h"
#include "log.h"
#include "packet_tools.h"
#include "acknowledge_packet.h"
#include "server_stat.h"

#include <errno.h>
#include <string.h>

/**
 * Notify all players on the server that a new player arrived.
 *
 * @param pl the player who arrived
 * @param s the server
 */
void s_notify_new_player(struct player *pl)
{
	struct player *tmp_pl;
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	data_size = 24 + player_to_data_size(pl);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_new_player, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CREATE_PL;	ptr += 2;	/* */
	/* public and private ID */		ptr += 8;	/* done later */
	/* counter */				ptr += 4;	/* done later */
	/* packet version */			ptr += 4;	/* empty for now */
	/* empty checksum */			ptr += 4;	/* done later */
	player_to_data(pl, ptr);
	
	/* customize and send for each player on the server */
	ar_each(struct player *, tmp_pl, iter, s->players)
		if (tmp_pl != pl) {
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
		}
	ar_end_each;
	free(data);
}

/**
 * Send a "player disconnected" message to all players.
 *
 * @param p the player who left
 */
void s_notify_player_left(struct player *p)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	struct server *s = p->in_chan->in_server;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_left, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERLEFT;	ptr += 2;	/* */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	*(uint32_t *)ptr = p->f0_s_counter;	ptr += 4;	/* packet counter */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = p->public_id;	ptr += 4;	/* ID of player who left */
	*(uint32_t *)ptr = 1;			ptr += 4;	/* visible notification */
	/* 32 bytes of garbage?? */		ptr += 32;	/* maybe some message ? */

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
 * Handles a disconnection request.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the client
 * @param cli_len the size of cli_addr
 * // F0 BE 2C 01
 */
void *c_req_leave(char *data, unsigned int len, struct player *pl)
{
	if (len != 24) {
		logger(LOG_WARN, "c_req_leave, packet has invalid size : %i instead of %i.", len, 24);
		return NULL;
	}
	send_acknowledge(pl);		/* ACK */
	/* send a notification to all players */
	s_notify_player_left(pl);
	remove_player(pl->in_chan->in_server, pl);

	return NULL;
}
