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
		logger(LOG_WARN, "send_message_to_all, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_MESSAGE;		ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = color;		ptr += 4;/* color of the message */
	*(uint8_t *)ptr = 0;			ptr += 1;/* type of msg (0 = all) */
	if (pl == NULL) {
		*(uint8_t *)ptr = 6;	ptr += 1;/* length of the sender's name */
		strncpy(ptr, "SERVER", *(ptr - 1));		ptr += 29;/* sender's name */
	} else {
		*(uint8_t *)ptr = MIN(29, strlen(pl->name));	ptr += 1;/* length of the sender's name */
		strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;/* sender's name */
	}
	strcpy(ptr, msg);

	ar_each(struct player *, tmp_pl, iter, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
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
		logger(LOG_WARN, "send_message_to_channel, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_MESSAGE;		ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = color;		ptr += 4;/* color of the message */
	*(uint8_t *)ptr = 1;			ptr += 1;/* type of msg (1 = channel) */
	*(uint8_t *)ptr = MIN(29, strlen(pl->name));	ptr += 1;/* length of the sender's name */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;/* sender's name */
	strcpy(ptr, msg);

	ar_each(struct player *, tmp_pl, iter, ch->players)
		*(uint32_t *)(data + 4) = tmp_pl->private_id;
		*(uint32_t *)(data + 8) = tmp_pl->public_id;
		*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
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
		logger(LOG_WARN, "send_message_to_player, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_MESSAGE;		ptr += 2;	/* */
	*(uint32_t *)ptr = tgt->private_id;	ptr += 4;/* private ID */
	*(uint32_t *)ptr = tgt->public_id;	ptr += 4;/* public ID */
	*(uint32_t *)ptr = tgt->f0_s_counter;	ptr += 4;/* public ID */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = color;		ptr += 4;/* color of the message */
	*(uint8_t *)ptr = 2;			ptr += 1;/* type of msg (2 = private) */
	*(uint8_t *)ptr = MIN(29, strlen(pl->name));	ptr += 1;/* length of the sender's name */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;/* sender's name */
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
	char *msg;
	struct channel *ch;
	struct player *tgt;
	
	send_acknowledge(pl);		/* ACK */

	memcpy(&color, data + 24, 4);
	memcpy(&msg_type, data + 28, 1);
	memcpy(&dst_id, data + 29, 4);
	msg = strdup(data + 33);

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
		logger(LOG_WARN, "Wrong type of message.");
	}
	free(msg);
	return NULL;
}
