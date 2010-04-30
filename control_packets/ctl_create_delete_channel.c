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
#include "acknowledge_packet.h"
#include "server_stat.h"
#include "database.h"

#include <errno.h>
#include <string.h>

/**
 * Notify players that a channel has been deleted
 *
 * @param s the server
 * @param del_id the id of the deleted channel
 */
static void s_notify_channel_deleted(struct server *s, uint32_t del_id)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 30;
	size_t iter;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_ERR, "s_notify_channel_deleted, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_CHANDELETE, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(del_id, &ptr);		/* ID of deleted channel */
	wu16(1, &ptr);			/* ????? the previous 
					   ptr += 2 is not an error*/

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

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
 * Notify a player that his request to delete a channel failed (channel not empty)
 *
 * @param s the server
 * @param pl the player who wants to delete the channel
 * @param pkt_cnt the counter of the packet we failed to execute
 */
static void s_resp_cannot_delete_channel(struct player *pl, uint32_t pkt_cnt)
{
	char *data, *ptr;
	int data_size = 30;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_ERR, "s_resp_cannot_delete_channel, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_CHANDELETE_ERROR, &ptr);	/* */
	wu32(pl->private_id, &ptr);	/* private ID */
	wu32(pl->public_id, &ptr);	/* public ID */
	wu32(pl->f0_s_counter, &ptr);	/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu16(0x00d1, &ptr);	/* ID of player who switched */
	wu32(pkt_cnt, &ptr);	/* ??? */

	/* check we filled all the packet */
	assert((ptr - data) == data_size);

	packet_add_crc_d(data, data_size);

	send_to(s, data, data_size, 0, pl);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Handles a request by a client to delete a channel.
 * This request will fail if there still are people in the channel.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_adr
 */
void *c_req_delete_channel(char *data, unsigned int len, struct player *pl)
{
	struct channel *del;
	uint32_t pkt_cnt, del_id;
	struct server *s = pl->in_chan->in_server;
	char *ptr = data + 12;

	pkt_cnt = ru32(&ptr);
	ptr = data + 24;
	del_id = ru32(&ptr);
	del = get_channel_by_id(s, del_id);

	send_acknowledge(pl);
	if (player_has_privilege(pl, SP_CHA_DELETE, del)) {
		if (del == NULL || del->players->used_slots > 0) {
			s_resp_cannot_delete_channel(pl, pkt_cnt);
		} else {
			/* if the channel is registered, remove it from the db */
			logger(LOG_INFO, "Flags : %i", ch_getflags(del));
			if ((ch_getflags(del) & CHANNEL_FLAG_UNREGISTERED) == 0)
				db_unregister_channel(s->conf, del);
			s_notify_channel_deleted(s, del_id);
			destroy_channel_by_id(s, del->id);
		}
	}
	return NULL;
}

/**
 * Notify all players on a server that a new channel has been created
 *
 * @param ch the new channel
 * @param creator the player who created the channel
 */
static void s_notify_channel_created(struct channel *ch, struct player *creator)
{
	char *data, *ptr;
	int data_size;
	struct player *tmp_pl;
	struct server *s = ch->in_server;
	size_t iter;

	data_size = 24 + 4;
	data_size += channel_to_data_size(ch);

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_ERR, "s_notify_channel_created, packet allocation failed : %s.", strerror(errno));
		return;
	}
	ptr = data;


	wu16(PKT_TYPE_CTL, &ptr);	/* */
	wu16(CTL_CREATE_CH, &ptr);	/* */
	ptr += 4;			/* private ID */
	ptr += 4; 			/* public ID */
	ptr += 4;			/* packet counter */
	ptr += 4;			/* packet version */
	ptr += 4;			/* empty checksum */
	wu32(creator->public_id, &ptr);	/* id of creator */
	channel_to_data(ch, ptr);

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
 * Handle a player request to create a new channel
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the creation
 */
void *c_req_create_channel(char *data, unsigned int len, struct player *pl)
{
	struct channel *ch;
	size_t bytes_read;
	char *ptr, *pass;
	struct server *s;
	struct channel *parent;
	int priv_nok = 0;
	int flags;

	s = pl->in_chan->in_server;
	send_acknowledge(pl);

	ptr = data + 24;
	bytes_read = channel_from_data(ptr, len - (ptr - data), &ch);
	ptr += bytes_read;
	pass = rstaticstring(29, &ptr);
	strcpy(ch->password, pass);
	free(pass);

	flags = ch_getflags(ch);
	/* Check the privileges */
	/* TODO : when we support subchannels, context will have to
	 * be changed to the parent channel (NULL if we create the
	 * channel at the root */
	if (flags & CHANNEL_FLAG_UNREGISTERED) {
		if (!player_has_privilege(pl, SP_CHA_CREATE_UNREGISTERED, NULL))
			priv_nok++;
	} else {
		if (!player_has_privilege(pl, SP_CHA_CREATE_REGISTERED, NULL))
			priv_nok++;
	}
	if (flags & CHANNEL_FLAG_DEFAULT
			&& !player_has_privilege(pl, SP_CHA_CREATE_DEFAULT, NULL))
		priv_nok++;
	if (flags & CHANNEL_FLAG_MODERATED
			&& !player_has_privilege(pl, SP_CHA_CREATE_MODERATED, NULL))
		priv_nok++;
	if (flags & CHANNEL_FLAG_SUBCHANNELS
			&& !player_has_privilege(pl, SP_CHA_CREATE_SUBCHANNELED, NULL))
		priv_nok++;

	if (priv_nok == 0) {
		add_channel(s, ch);
		if (ch->parent_id != 0) {
			parent = get_channel_by_id(s, ch->parent_id);
			channel_add_subchannel(parent, ch);
			/* if the parent is registered, register this one */
			if (!(ch_getflags(parent) & CHANNEL_FLAG_UNREGISTERED)) {
				db_register_channel(s->conf, ch);
			}
		}
		logger(LOG_INFO, "New channel created");
		print_channel(ch);
		if (! (ch_getflags(ch) & CHANNEL_FLAG_UNREGISTERED))
			db_register_channel(s->conf, ch);
		print_channel(ch);
		s_notify_channel_created(ch, pl);
	}
	return NULL;
}
