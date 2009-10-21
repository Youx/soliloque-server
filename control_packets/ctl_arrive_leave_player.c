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

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_CREATE_PL, &ptr);
	/* public and private ID */		ptr += 8;	/* done later */
	/* counter */				ptr += 4;	/* done later */
	/* packet version */			ptr += 4;	/* empty for now */
	/* empty checksum */			ptr += 4;	/* done later */
	player_to_data(pl, ptr);
	
	/* customize and send for each player on the server */
	ar_each(struct player *, tmp_pl, iter, s->players)
		if (tmp_pl != pl) {
			ptr = data + 4;
			wu32(tmp_pl->private_id, &ptr);
			wu32(tmp_pl->public_id, &ptr);
			wu32(tmp_pl->f0_s_counter, &ptr);
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0, tmp_pl);
			tmp_pl->f0_s_counter++;
		}
	ar_end_each;
	free(data);
}

void s_notify_server_stopping(struct server *s)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_notify_player_left, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ar_each(struct player *, tmp_pl, iter, s->players)
		ptr = data;

		wu16(PKT_TYPE_CTL, &ptr);
		wu16(CTL_PLAYERLEFT, &ptr);
		wu32(tmp_pl->private_id, &ptr);		/* private ID */
		wu32(tmp_pl->public_id, &ptr);		/* public ID */
		wu32(tmp_pl->f0_s_counter, &ptr);	/* packet counter */
		ptr += 4;				/* packet version */
		ptr += 4;				/* empty checksum */
		wu32(tmp_pl->public_id, &ptr);		/* ID of player who left */
		wu32(4, &ptr);				/* 4 = server stopping */
		ptr += 32;				/* 32 bytes of garbage?? */

		/* check we filled all the packet */
		assert((ptr - data) == data_size);

		packet_add_crc_d(data, data_size);
		send_to(s, data, data_size, 0, tmp_pl);
		tmp_pl->f0_s_counter++;
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

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_PLAYERLEFT, &ptr);
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(p->public_id, &ptr); 	/* ID of player who left */
	wu32(1, &ptr);			/* visible notification */
	ptr += 32;			/* 32 bytes of garbage?? */

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
