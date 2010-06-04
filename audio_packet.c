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

#include "audio_packet.h"

#include "player.h"
#include "server.h"
#include "channel.h"
#include "array.h"
#include "server_stat.h"
#include "log.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

/** The size of the raw audio block (in bytes) */
static size_t codec_audio_size[13] = {153, 51, 165, 132, 0, 27, 50, 75, 100, 138, 188, 228, 308};
/** The number of frames contained in one block */
static size_t codec_nb_frames[13] = { 9, 3, 5, 4,	0, 5, 5, 5, 5, 5, 5, 5, 5};
/** The offset of the audio block after the 16 bytes of header */
static size_t codec_offset[13] = {6, 6, 6, 6, 0, 1, 1, 1, 1, 1, 1, 1, 1};


/**
 * Handle a received audio packet by sending its audio
 * block to all the players in the same channel.
 *
 * @param in the received packet
 * @param len size of the received packet
 *
 * @return 0 on success, -1 on failure.
 */
int audio_received(char *in, size_t len, struct server *s)
{
	uint32_t pub_id, priv_id;
	uint8_t data_codec;

	struct player *sender;
	struct channel *ch_in;
	struct player *tmp_pl;

	size_t data_size, audio_block_size, expected_size;
	ssize_t err;
	size_t iter;
	char *data, *ptr, *ptrin;
	
	if (len < 12) {
		WARNING("audio_received, size is not enough for metadata (%i)", len);
		return -1;
	}
	ptrin = in;
	ptrin += 3;
	data_codec = ru8(&ptrin);
	priv_id = ru32(&ptrin);
	pub_id = ru32(&ptrin);
	if (data_codec > 12) {
		WARNING("invalid codec ID sent : %i", data_codec);
		return -1;
	}
	
	sender = get_player_by_ids(s, pub_id, priv_id);

	if (sender != NULL) {
		sender->stats->activ_time = time(NULL);	/* update */
		ch_in = sender->in_chan;
		/* Security checks */
		if (data_codec != ch_in->codec) {
			WARNING("Player sent a wrong codec ID : %" PRIu8 ", expected : %" PRIu8 ".", data_codec, ch_in->codec);
			return -1;
		}

		audio_block_size = codec_offset[(int)data_codec] + codec_audio_size[(int)data_codec];
		expected_size = 16 + audio_block_size;
		if (len != expected_size) {
			WARNING("Audio packet's size is incorrect : %zu bytes, expected : %zu.", len,
					expected_size);
			return -1;
		}

		/* Initialize the packet we want to send */
		data_size = len + 6; /* we will add the id of player sending */
		data = (char *)calloc(data_size, sizeof(char));
		if (data == NULL) {
			ERROR("audio_received, could not allocate packet : %s.", strerror(errno));
			return -1;
		}
		ptr = data;
		wu16(0xbef3, &ptr); 			/* function code */
		/* 1 byte empty */				ptr += 1;		/* NULL */
		wu8(ch_in->codec, &ptr);		/* codec */
		/* private ID */				ptr += 4;		/* empty yet */
		/* public ID */					ptr += 4;		/* empty yet */
		wu16(0, &ptr);				/* unknown, maybe server conversation ID? */
		wu16(*(uint16_t *)(in + 14), &ptr);	/* counter */
		wu32(sender->public_id, &ptr);		/* ID of sender */
		wu16(*(uint16_t *)(in + 12), &ptr);	/* conversation counter */
		/* audio data */
		memcpy(ptr, in + 16, audio_block_size);
		ptr += audio_block_size;

		/* assert we filled the whole packet */
		assert((ptr - data) == data_size);

		ar_each(struct player *, tmp_pl, iter, ch_in->players)
			if (tmp_pl != sender && !ar_has(tmp_pl->muted, sender)) {
				ptr = data + 4;
				wu32(tmp_pl->private_id, &ptr);
				wu32(tmp_pl->public_id, &ptr);
				err = sendto(sender->in_chan->in_server->socket_desc, data, data_size, 0,
						(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
				if (err == -1) {
					ERROR("audio_received, could not send packet : %s.", strerror(errno));
				}
			}
		ar_end_each;
		free(data);
		return 0;
	} else {
		WARNING("Wrong public/private ID pair : %x/%x.", pub_id, priv_id);
		return -1;
	}
}
