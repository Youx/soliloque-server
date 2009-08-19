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

#include "packet_sender.h"
#include "server.h"
#include "queue.h"
#include "player.h"
#include "log.h"
#include "server_stat.h"
#include "packet_tools.h"

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

static void send_curr_packet(struct player *p, struct server *s)
{
	char *packet;
	size_t p_size;
	int ret;

	packet = peek_at_queue(p->packets);
	if (packet != NULL) {
		p_size = peek_at_size(p->packets);

		/* add packet to server statistics */
		sstat_add_packet(s->stats, p_size, 1);
		logger(LOG_INFO, "Really sending packet type 0x%x", *(uint32_t *)packet);
		ret = sendto(s->socket_desc, packet, p_size, 0,
				(struct sockaddr *)p->cli_addr, p->cli_len);
		if (ret == -1)
			logger(LOG_WARN, "send_curr_packet failed : %s", strerror(errno));
		/* update packet version counter */
		(*(uint16_t *)(packet + 16))++;
		/* update checksum */
		packet_add_crc_d(packet, p_size);
	}
}

void *packet_sender_thread(void *args)
{
	struct server *s;
	struct player *p;
	struct timeval now, diff, *last_sent;
	size_t iter;

	s = (struct server *)args;
	while(1) {
		gettimeofday(&now, NULL);
		/* Can we */
		/* sending their packet to active players */
		ar_each(struct player *, p, iter, s->players)
			pthread_mutex_lock(&p->packets->mutex);
			last_sent = queue_get_time(p->packets);
			if (last_sent != NULL) {
				timersub(&now, last_sent, &diff);
				/* resend a packet every 0.5s */
				if (diff.tv_sec > 0 || diff.tv_usec > 500000) {
					queue_update_time(p->packets);
					send_curr_packet(p, s);
				}
			}
			pthread_mutex_unlock(&p->packets->mutex);
		ar_end_each;

		/* sending their last packets to leaving players */
		ar_each(struct player *, p, iter, s->leaving_players)
			pthread_mutex_lock(&p->packets->mutex);
			last_sent = queue_get_time(p->packets);
			if (last_sent != NULL) {
				timersub(&now, last_sent, &diff);
				if (diff.tv_sec > 0 || diff.tv_usec > 500000) {
					queue_update_time(p->packets);
					send_curr_packet(p, s);
				}
			}
			pthread_mutex_unlock(&p->packets->mutex);
			/* if there is no more packets in the queue,
			 * we can safely destroy this player */
			if (p->packets->first == NULL) {
				ar_remove(s->leaving_players, p);
				destroy_player(p);
			}
		ar_end_each;

		usleep(50000);
	}
}

