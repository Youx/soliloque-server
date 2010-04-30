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
		logger(LOG_ERR, "s_resp_chans, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;
	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_LIST_CH, &ptr);	/* */
	wu32(pl->private_id, &ptr);	/* player private id */
	wu32(pl->public_id, &ptr);	/* player public id */
	wu32(pl->f0_s_counter, &ptr);	/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	wu32(s->chans->used_slots, &ptr);	/* number of channels sent */
	/* dump the channels to the packet */
	ar_each(struct channel *, ch, iter, s->chans)
		ch_size = channel_to_data_size(ch);
		channel_to_data(ch, ptr);
		ptr += ch_size;
	ar_end_each;

	packet_add_crc_d(data, data_size);

	logger(LOG_DBG, "size of all channels : %i", data_size);
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
		logger(LOG_ERR, "s_resp_players, packet allocation failed : %s.", strerror(errno));
		return;
	}
	while (nb_players > 0) {
		bzero(data, data_size * sizeof(char));
		ptr = data;
		/* initialize the packet */
		wu16(PKT_TYPE_CTL, &ptr);	/* */
		wu16(CTL_LIST_PL, &ptr);	/* */
		wu32(pl->private_id, &ptr);	/* player private id */
		wu32(pl->public_id, &ptr);	/* player public id */
		wu32(pl->f0_s_counter, &ptr);	/* packet counter */
		ptr += 4;			/* packet version */
		ptr += 4;			/* empty checksum */
		wu32(MIN(10, nb_players), &ptr);
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

		logger(LOG_DBG, "size of all players : %i", data_size);
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
		logger(LOG_ERR, "s_resp_unknown, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;
	/* initialize the packet */
	wu32(0x0008bef0, &ptr);
	wu32(pl->private_id, &ptr);		/* player private id */
	wu32(pl->public_id, &ptr);		/* player public id */
	wu32(pl->f0_s_counter, &ptr);		/* packet counter */
	ptr += 4;				/* packet version */
	ptr += 4;				/* empty checksum */
	ptr += 256;				/* empty data ??? */
	wu8(0x6e, &ptr);
	wu8(0x61, &ptr);
	ptr += 1;

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

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
