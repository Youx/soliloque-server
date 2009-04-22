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

#include "acknowledge_packet.h"
#include "player.h"
#include "server_stat.h"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/**
 * Send an acknowledge packet to a player
 * ACK packets consist of just a 16 bytes header
 *
 * @param pl the player
 */
void send_acknowledge(struct player *pl)
{
	char *data, *ptr;
	size_t data_size = 16;
	ssize_t err;

	data = (void *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		printf("(EE) send_acknowledge, packet data allocation failed : %s.\n", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_ACK;	ptr+=2;
	*(uint16_t *)ptr = 0x0000;		ptr+=2;
	*(uint32_t *)ptr = pl->private_id;	ptr+=4;
	*(uint32_t *)ptr = pl->public_id;	ptr+=4;
	*(uint32_t *)ptr = pl->f1_s_counter;	ptr+=4;

	err = send_to(pl->in_chan->in_server, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	if (err == -1) {
		printf("(EE) send_acknowledge, sending data failed : %s.\n", strerror(errno));
	}
	pl->f1_s_counter++;
	free(data);
}
