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
	*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;/* */
	*(uint16_t *)ptr = CTL_SERVSTATS;		ptr += 2;/* */
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	*(uint64_t *)ptr = time(NULL) - s->stats->start_time;	ptr += 8;/* server uptime */
	*(uint16_t *)ptr = 501;				ptr += 2;/* server version */
	*(uint16_t *)ptr = 0;				ptr += 2;/* server version */
	*(uint16_t *)ptr = 2;				ptr += 2;/* server version */
	*(uint16_t *)ptr = 0;				ptr += 2;/* server version */
	*(uint32_t *)ptr = s->players->used_slots;	ptr += 4;/* number of players connected */
	*(uint64_t *)ptr = s->stats->pkt_sent;		ptr += 8;/* total bytes received */
	*(uint64_t *)ptr = s->stats->size_sent;		ptr += 8;/* total bytes sent */
	*(uint64_t *)ptr = s->stats->pkt_rec;		ptr += 8;/* total packets received */
	*(uint64_t *)ptr = s->stats->size_rec;		ptr += 8;/* total packets sent */
	*(uint32_t *)ptr = stats[0];			ptr += 4;/* bytes received/sec (last second) */
	*(uint32_t *)ptr = stats[1];			ptr += 4;/* bytes sent/sec (last second) */
	*(uint32_t *)ptr = stats[2]/60;			ptr += 4;/* bytes received/sec (last minute) */
	*(uint32_t *)ptr = stats[3]/60;			ptr += 4;/* bytes sent/sec (last minute) */
	*(uint64_t *)ptr = s->stats->total_logins;	ptr += 8;/* total logins */

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

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERSTATS;	ptr += 2;	/* */
	*(uint32_t *)ptr = pl->private_id;	ptr += 4;/* private ID */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;/* public ID */
	*(uint32_t *)ptr = pl->f0_s_counter;	ptr += 4;/* packet counter */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */

	*(uint32_t *)ptr = tgt->public_id;	ptr += 4;/* player we get the info of */
	*(uint32_t *)ptr = time(NULL) - tgt->stats->start_time;	ptr += 4;/* time connected */
	*(uint16_t *)ptr = tgt->stats->pkt_lost / (tgt->stats->pkt_sent + 1 + tgt->stats->pkt_rec);
	*(uint32_t *)ptr = 0;			ptr += 4;/* ping */
	*(uint16_t *)ptr = time(NULL) - tgt->stats->activ_time;	ptr += 2;/* time iddle */
						ptr += 2;/* packet loss */
	*(uint16_t *)ptr = pl->version[0];	ptr += 2;/* client version */
	*(uint16_t *)ptr = pl->version[1];	ptr += 2;/* client version */
	*(uint16_t *)ptr = pl->version[2];	ptr += 2;/* client version */
	*(uint16_t *)ptr = pl->version[3];	ptr += 2;/* client version */
	*(uint32_t *)ptr = tgt->stats->pkt_sent;ptr += 4;/* packets sent */
	*(uint32_t *)ptr = tgt->stats->pkt_rec;	ptr += 4;/* packets received */
	*(uint32_t *)ptr = tgt->stats->size_sent;	ptr += 4;/* bytes sent */
	*(uint32_t *)ptr = tgt->stats->size_rec;ptr += 4;/* bytes received */
	*ptr = MIN(strlen(ip), 29);				ptr += 1;/* size of ip */
	strncpy(ptr, ip, *(ptr - 1));		ptr += 29;/* ip of client */
	*ptr = MIN(strlen(tgt->login), 29);	ptr += 1;/* size of login */
	strncpy(ptr, tgt->login, *(ptr - 1));	ptr += 29;/* login */
	*(uint32_t *)ptr = tgt->in_chan->id;	ptr += 4;/* id of channel */
	*(uint16_t *)ptr = player_get_channel_privileges(tgt, tgt->in_chan);	ptr += 2;/* channel privileges */
	*(uint16_t *)ptr = tgt->global_flags;	ptr += 2;/* global flags */
	*ptr = MIN(strlen(tgt->machine), 29);	ptr += 1;/* size of platform */
	strncpy(ptr, tgt->machine, *(ptr - 1));	ptr += 29;/*platform */

	packet_add_crc_d(data, data_size);
	send_to(pl->in_chan->in_server, data, data_size, 0, pl);
	pl->f0_s_counter++;

	free(data);
	free(ip);
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

	s = pl->in_chan->in_server;
	send_acknowledge(pl);

	tgt_id = *(uint32_t *)(data + 24);
	tgt = get_player_by_public_id(s, tgt_id);

	if (tgt != NULL) {
		s_res_player_stats(pl, tgt);
	} else {
		logger(LOG_INFO, "TGT = NULL");
	}

	return NULL;
}
