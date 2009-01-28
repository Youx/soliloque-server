#include "audio_packet.h"

#include "player.h"
#include "server.h"
#include "channel.h"
#include "array.h"
#include "server_stat.h"

#include <string.h>
#include <stdlib.h>

/** The size of the raw audio block (in bytes) */
int codec_audio_size[13] = {153, 51, 165, 132, 0, 27, 50, 75, 100, 138, 188, 228, 308};
/** The number of frames contained in one block */
int codec_nb_frames[13] = { 9, 3, 5, 4,	0, 5, 5, 5, 5, 5, 5, 5, 5};
/** The offset of the audio block after the 16 bytes of header */
int codec_offset[13] = {6, 6, 6, 6, 0, 1, 1, 1, 1, 1, 1, 1, 1};


/**
 * Handle a received audio packet by sending its audio
 * block to all the players in the same channel.
 *
 * @param in the received packet
 * @param len size of the received packet
 *
 * @return 0 on success, -1 on failure.
 */
int audio_received(char *in, int len, struct server *s)
{
	uint32_t pub_id, priv_id;
	char data_codec;

	struct player *sender;
	struct channel *ch_in;
	struct player *tmp_pl;

	size_t data_size, audio_block_size, expected_size;
	int i;
	char *data, *ptr;
	
	data_codec = in[3];
	
	memcpy(&priv_id, in + 4, 4);
	memcpy(&pub_id, in + 8, 4);
	sender = get_player_by_ids(s, pub_id, priv_id);

	if (sender != NULL) {
		ch_in = sender->in_chan;
		/* Security checks */
		if (data_codec != ch_in->codec) {
			printf("(EE) Player sent a wrong codec ID : %i, expected : %i.\n", 
					data_codec, ch_in->codec);
			return -1;
		}

		audio_block_size = codec_offset[(int)data_codec] + codec_audio_size[(int)data_codec];
		expected_size = 16 + audio_block_size;
		if (len != expected_size) {
			printf("(EE) Audio packet's size is incorrect : %i bytes, expected : %i.\n", len,
					expected_size);
			return -1;
		}

		/* Initialize the packet we want to send */
		data_size = len + 6; /* we will add the id of player sending */
		data = (char *)calloc(data_size, sizeof(char));
		ptr = data;

		*(uint16_t *)ptr = 0xbef3;			ptr += 2;		/* function code */
		/* 1 byte empty */				ptr += 1;		/* NULL */
		*(uint8_t *)ptr = ch_in->codec;			ptr += 1;		/* codec */
		/* private ID */				ptr += 4;		/* empty yet */
		/* public ID */					ptr += 4;		/* empty yet */
		*(uint16_t *)ptr = *(uint16_t *)(in + 12);	ptr += 2;		/* counter */
		*(uint16_t *)ptr = *(uint16_t *)(in + 14);	ptr += 2;		/* conversation counter */
		*(uint32_t *)ptr = pub_id;			ptr += 4;		/* ID of sender */
								ptr += 2;		/* another counter?? */
		memcpy(ptr, in + 16, audio_block_size);		ptr += audio_block_size;
		
		for (i = 0 ; i < ch_in->max_users ; i++) {
			tmp_pl = ch_in->players[i];
			if (tmp_pl != NULL && tmp_pl != sender) {
				*(uint32_t *)(data + 4) = tmp_pl->private_id;
				*(uint32_t *)(data + 8) = tmp_pl->public_id;
				send_to(sender->in_chan->in_server, data, data_size, 0, 
						(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			}
		}
		free(data);
		return 0;
	} else {
		printf("(EE) Wrong public/private ID pair : %x/%x.\n", pub_id, priv_id);
		return -1;
	}
}
