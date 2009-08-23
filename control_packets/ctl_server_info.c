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

/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the channels.
 *
 * @param pl the player we send the channel list to
 * @param s the server we will get the channels from
 *
 * TODO : split the channels over packets
 */
static void s_resp_chans(struct player *pl)
{
	char *data;
	int data_size = 0;
	struct channel *ch;
	char *ptr;
	int ch_size;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	/* compute the size of the packet */
	data_size += 24;	/* header */
	data_size += 4;		/* number of channels in packet */
	ar_each(struct channel *, ch, iter, s->chans)
		data_size += channel_to_data_size(ch);
	ar_end_each;

	/* initialize the packet */
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_chans, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;
	*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_LIST_CH;			ptr += 2;	/* */
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;	/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	*(uint32_t *)ptr = s->chans->used_slots;	ptr += 4;	/* number of channels sent */
	/* dump the channels to the packet */
	ar_each(struct channel *, ch, iter, s->chans)
		ch_size = channel_to_data_size(ch);
		channel_to_data(ch, ptr);
		ptr += ch_size;
	ar_end_each;

	packet_add_crc_d(data, data_size);

	logger(LOG_INFO, "size of all channels : %i", data_size);
	send_to(s, data, data_size, 0, pl);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the players.
 *
 * @param pl the player we send the player list to
 * @param s the server we will get the players from
 */
static void s_resp_players(struct player *pl)
{
	char *data;
	int data_size = 0;
	char *ptr;
	int p_size;
	int nb_players;
	struct player *pls[10];
	int i;
	int players_copied;
	struct server *s = pl->in_chan->in_server;

	/* compute the size of the packet */
	data_size += 24;	/* header */
	data_size += 4;		/* number of players in packet */
	data_size += 10 * player_to_data_size(NULL); /* players */

	nb_players = s->players->used_slots;
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_players, packet allocation failed : %s.", strerror(errno));
		return;
	}
	while (nb_players > 0) {
		bzero(data, data_size * sizeof(char));
		ptr = data;
		/* initialize the packet */
		*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
		*(uint16_t *)ptr = CTL_LIST_PL;			ptr += 2;	/* */
		*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* player private id */
		*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* player public id */
		*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;	/* packet counter */
		/* packet version */				ptr += 4;
		/* empty checksum */				ptr += 4;
		*(uint32_t *)ptr = MIN(10, nb_players);		ptr += 4;
		/* dump the players to the packet */
		bzero(pls, 10 * sizeof(struct player *));
		players_copied = ar_get_n_elems_start_at(s->players, 10,
				s->players->used_slots-nb_players, (void**)pls);
		for (i = 0 ; i < players_copied ; i++) {
			p_size = player_to_data_size(pls[i]);
			player_to_data(pls[i], ptr);
			ptr += p_size;
		}
		packet_add_crc_d(data, data_size);

		logger(LOG_INFO, "size of all players : %i", data_size);
		send_to(s, data, data_size, 0, pl);
		pl->f0_s_counter++;
		/* decrement the number of players to send */
		nb_players -= MIN(10, nb_players);
		free(data);
	}
}

static void s_resp_unknown(struct player *pl)
{
	char *data;
	char *ptr;
	int data_size = 283;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_unknown, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;
	/* initialize the packet */
	*(uint32_t *)ptr = 0x0008bef0;			ptr += 4;
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;		/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;		/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;		/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	/* empty data ??? */				ptr += 256;
	*ptr = 0x6e;					ptr += 1;
	*ptr = 0x61;					ptr += 1;
							ptr += 1;

	packet_add_crc_d(data, data_size);

	send_to(s, data, data_size, 0, pl);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Handles a request for channels and players by the player.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the client
 * @param cli_len the size of cli_addr
 */
void *c_req_chans(char *data, unsigned int len, struct player *pl)
{
	if (len != 120) {
		logger(LOG_WARN, "c_req_chans, packet has an invalid size : %i instead of %i.", len, 120);
		return NULL;
	}
	send_acknowledge(pl);		/* ACK */
	s_resp_chans(pl);	/* list of channels */
	s_resp_players(pl);	/* list of players */
	s_resp_unknown(pl);

	return NULL;
}