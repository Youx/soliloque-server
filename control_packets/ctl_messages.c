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
 * Send a message to all players on the server
 *
 * @param pl the sender
 * @param color the hexadecimal color of the message
 * @param msg the message
 */
void send_message_to_all(struct player *pl, uint32_t color, char *msg)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	/* header size (24) + color (4) + type (1) + name size (1) + name (29) + msg (?) */
	data_size = 24 + 4 + 1 + 1 + 29 + (strlen(msg) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		ERROR("send_message_to_all, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_MESSAGE, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(color, &ptr);		/* color of the message */
	wu8(0, &ptr);			/* type of msg (0 = all) */
	if (pl == NULL) {
		wstaticstring("SERVER", 29, &ptr); /* sender's name */
	} else {
		wstaticstring(pl->name, 29, &ptr); /* sender's name */
	}
	strcpy(ptr, msg);

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
 * Send a message to all players in a channel
 *
 * @param pl the sender
 * @param ch the channel to send the message to
 * @param color the hexadecimal color of the message
 * @param msg the message
 */
static void send_message_to_channel(struct player *pl, struct channel *ch, uint32_t color, char *msg)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	size_t iter;

	/* header size (24) + color (4) + type (1) + name size (1) + name (29) + msg (?) */
	data_size = 24 + 4 + 1 + 1 + 29 + (strlen(msg) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		ERROR("send_message_to_channel, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_MESSAGE, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(color, &ptr);		/* color of the message */
	wu8(1, &ptr);			/* type of msg (1 = channel) */
	wstaticstring(pl->name, 29, &ptr);
	strcpy(ptr, msg);

	ar_each(struct player *, tmp_pl, iter, ch->players)
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
 * Send a message to a specific player
 *
 * @param pl the sender
 * @param tgt the receiver
 * @param color the hexadecimal color of the message
 * @param msg the message
 */
static void send_message_to_player(struct player *pl, struct player *tgt, uint32_t color, char *msg)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	/* header size (24) + color (4) + type (1) + name size (1) + name (29) + msg (?) */
	data_size = 24 + 4 + 1 + 1 + 29 + (strlen(msg) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		ERROR("send_message_to_player, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_MESSAGE, &ptr);	/* */
	wu32(tgt->private_id, &ptr);	/* private ID */
	wu32(tgt->public_id, &ptr);	/* public ID */
	wu32(tgt->f0_s_counter, &ptr);	/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(color, &ptr);		/* color of the message */
	wu8(2, &ptr);			/* type of msg (2 = private) */
	wstaticstring(pl->name, 29, &ptr);/* length of the sender's name */
	strcpy(ptr, msg);

	packet_add_crc_d(data, data_size);
	send_to(s, data, data_size, 0, tgt);
	tgt->f0_s_counter++;
	free(data);
}

/**
 * Handle a player request to send a message
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player sending the message
 */
void *c_req_send_message(char *data, unsigned int len, struct player *pl)
{
	uint32_t color;
	char msg_type;
	uint32_t dst_id;
	char *msg, *ptr;
	struct channel *ch;
	struct player *tgt;
	
	send_acknowledge(pl);	/* ACK */

	ptr = data + 24;
	color = ru32(&ptr);
	msg_type = ru8(&ptr);
	dst_id = ru32(&ptr);
	msg = strdup(ptr);	/* fixme */

	switch (msg_type) {
	case 0:
		if (player_has_privilege(pl, SP_OTHER_TEXT_ALL, NULL))
			send_message_to_all(pl, color, msg);
		break;
	case 1:
		ch = get_channel_by_id(pl->in_chan->in_server, dst_id);
		/* If he is in the channel, check if he can send msg to his channel or any channel
		 * if he is not in the channel, check if he can send msg to any channel */
		if (ch != NULL) {
			if ((ch == pl->in_chan && player_has_privilege(pl, SP_OTHER_TEXT_IN_CH, ch))
					|| player_has_privilege(pl, SP_OTHER_TEXT_ALL_CH, ch))
			send_message_to_channel(pl, ch, color, msg);
		}
		break;
	case 2:
		tgt = get_player_by_public_id(pl->in_chan->in_server, dst_id);
		if (player_has_privilege(pl, SP_OTHER_TEXT_PL, tgt->in_chan))
				send_message_to_player(pl, tgt, color, msg);
		break;
	default:
		WARNING("c_req_send_message - wrong type of message : %i.", msg_type);
	}
	free(msg);
	return NULL;
}
