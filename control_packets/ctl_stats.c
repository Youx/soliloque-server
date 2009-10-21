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
 * Send the server information to the player
 *
 * @param pl the player who requested the server info
 */
static void s_resp_server_stats(struct player *pl)
{
	int data_size = 100;
	char *data, *ptr;
	struct server *s = pl->in_chan->in_server;
	uint32_t stats[4] = {0, 0, 0, 0};
	
	compute_timed_stats(s->stats, stats);
	/* initialize the packet */
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_resp_server_stats, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;
	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_SERVSTATS, &ptr);
	wu32(pl->private_id, &ptr);			/* player private id */
	wu32(pl->public_id, &ptr);			/* player public id */
	wu32(pl->f0_s_counter, &ptr);			/* packet counter */
	ptr += 4;					/* packet version */
	ptr += 4;					/* empty checksum */
	wu64(time(NULL) - s->stats->start_time, &ptr);	/* server uptime */
	wu16(501, &ptr);				/* server version */
	wu16(0, &ptr);					/* server version */
	wu16(2, &ptr);					/* server version */
	wu16(0, &ptr);					/* server version */
	wu32(s->players->used_slots, &ptr);		/* number of players connected */
	wu64(s->stats->pkt_sent, &ptr);			/* total bytes received */
	wu64(s->stats->size_sent, &ptr);		/* total bytes sent */
	wu64(s->stats->pkt_rec, &ptr);			/* total packets received */
	wu64(s->stats->size_rec, &ptr);			/* total packets sent */
	wu32(stats[0], &ptr);				/* bytes received/sec (last second) */
	wu32(stats[1], &ptr);				/* bytes sent/sec (last second) */
	wu32(stats[2]/60, &ptr);			/* bytes received/sec (last minute) */
	wu32(stats[3]/60, &ptr);			/* bytes sent/sec (last minute) */
	wu64(s->stats->total_logins, &ptr);		/* total logins */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	packet_add_crc_d(data, data_size);

	send_to(s, data, data_size, 0, pl);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Handle a player request for server information
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_server_stats(char *data, unsigned int len, struct player *pl)
{
	send_acknowledge(pl);		/* ACK */
	s_resp_server_stats(pl);
	return NULL;
}

/**
 * Send player connection statistics to another player.
 *
 * @param pl the player asking for it
 * @param tgt the player whose statistics we want
 */
static void s_res_player_stats(struct player *pl, struct player *tgt)
{
	int data_size;
	char *data, *ptr;
	char *ip;

	data_size = 164;
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_WARN, "s_res_player_stats, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;
	ip = inet_ntoa(tgt->cli_addr->sin_addr);

	wu16(PKT_TYPE_CTL, &ptr);
	wu16(CTL_PLAYERSTATS, &ptr);
	wu32(pl->private_id, &ptr);			/* private ID */
	wu32(pl->public_id, &ptr);			/* public ID */
	wu32(pl->f0_s_counter, &ptr);			/* packet counter */
	ptr += 4;					/* packet version */
	ptr += 4;					/* empty checksum */

	wu32(tgt->public_id, &ptr);			/* player we get the info of */
	wu32(time(NULL) - tgt->stats->start_time, &ptr);/* time connected */
	wu16(tgt->stats->pkt_lost * 100 / (tgt->stats->pkt_sent + 1 + tgt->stats->pkt_rec), &ptr);
	wu32(12, &ptr);					/* ping */
	wu16(time(NULL) - tgt->stats->activ_time, &ptr);/* time iddle */
	wu16(pl->version[0], &ptr);			/* client version */
	wu16(pl->version[1], &ptr);			/* client version */
	wu16(pl->version[2], &ptr);			/* client version */
	wu16(pl->version[3], &ptr);			/* client version */
	wu32(tgt->stats->pkt_sent, &ptr);		/* packets sent */
	wu32(tgt->stats->pkt_rec, &ptr);		/* packets received */
	wu32(tgt->stats->size_sent, &ptr);		/* bytes sent */
	wu32(tgt->stats->size_rec, &ptr);		/* bytes received */
	wstaticstring(ip, 29, &ptr);			/* ip of client */
	wu16(0, &ptr);					/* port of client */
	wstaticstring(tgt->login, 29, &ptr);		/* login */
	wu32(tgt->in_chan->id, &ptr);			/* id of channel */
	wu16(player_get_channel_privileges(tgt, tgt->in_chan), &ptr);/* channel privileges */
	wu16(tgt->global_flags, &ptr);			/* global flags */
	wstaticstring(tgt->machine, 29, &ptr);		/* platform */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	packet_add_crc_d(data, data_size);
	send_to(pl->in_chan->in_server, data, data_size, 0, pl);
	pl->f0_s_counter++;

	free(data);
}

/**
 * Handles a request for player connection statistics.
 *
 * @param data the request packet
 * @param len the length of the packet
 * @param pl the player asking for it
 */
void *c_req_player_stats(char *data, unsigned int len, struct player *pl)
{
	struct server *s;
	uint32_t tgt_id;
	struct player *tgt;
	char *ptr;

	s = pl->in_chan->in_server;
	send_acknowledge(pl);

	ptr = data + 24;
	tgt_id = ru32(&ptr);
	tgt = get_player_by_public_id(s, tgt_id);

	if (tgt != NULL) {
		s_res_player_stats(pl, tgt);
	} else {
		logger(LOG_INFO, "TGT = NULL");
	}

	return NULL;
}
