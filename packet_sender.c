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

#include <pthread.h>
#include <errno.h>

static void send_curr_packet(struct player *p)
{
	char *packet;
	size_t p_size;
	struct server *s = p->in_chan->in_server;
	uint16_t packet_version;
	int ret;

	packet = peek_at_queue(p->packets);
	if (packet != NULL) {
		p_size = peek_at_size(p->packets);

		/* add packet to server statistics */
		sstat_add_packet(s->stats, p_size, 1);
		logger(LOG_INFO, "Really sending packet type 0x%x", *(uint32_t *)packet);
		ret = sendto(s->socket_desc, packet, p_size, 0,
				p->cli_addr, p->cli_len);
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
	int socket_desc;
	struct player *p;
	size_t iter;

	s = (struct server *)args;
	while(1) {
		/* Can we */
		/*sem_wait(&s->send_packets);*/
		ar_each(struct player *, p, iter, s->players)
			pthread_mutex_lock(&p->packets->mutex);
			send_curr_packet(p);
			pthread_mutex_unlock(&p->packets->mutex);
		ar_end_each;
		usleep(50000);
	}
}

