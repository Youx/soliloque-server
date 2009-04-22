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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "player.h"
#include "server.h"
#include "compat.h"
#include "audio_packet.h"
#include "player_channel_privilege.h"


#define CHANNEL_FLAG_REGISTERED   0
#define CHANNEL_FLAG_UNREGISTERED 1
#define CHANNEL_FLAG_MODERATED    2
#define CHANNEL_FLAG_PASSWORD   4
#define CHANNEL_FLAG_SUBCHANNELS  8
#define CHANNEL_FLAG_DEFAULT    16


struct channel {
	uint32_t id;
	uint16_t flags;
	uint8_t codec;
	uint16_t sort_order;
	
	char *name;
	char *topic;
	char *desc;
	char password[30];

	struct array *players;
	struct server *in_server;
	/* channel tree */
	struct array *subchannels;
	/* player privileges */
	struct array *pl_privileges;

	struct channel *parent;
	uint32_t parent_id;

	uint32_t db_id;
};



/* Channel functions */
struct channel *new_channel(char *name, char *topic, char *desc, uint16_t flags,
		uint16_t codec, uint16_t sort_order, uint16_t max_users);
struct channel *new_predef_channel(void);
int destroy_channel(struct channel *chan);

int add_player_to_channel(struct channel *chan, struct player *player);

void print_channel(struct channel *chan);

int channel_to_data(struct channel *ch, char *data);
int channel_to_data_size(struct channel *ch);
size_t channel_from_data(char *data, int len, struct channel **dst);
/* subchannels */
int channel_remove_subchannel(struct channel *ch, struct channel *subchannel);
int channel_add_subchannel(struct channel *ch, struct channel *subchannel);

int ch_getflags(struct channel *ch);
char *ch_getpass(struct channel *ch);
char ch_isfull(struct channel *ch);
struct player_channel_privilege *get_player_channel_privilege(struct player *pl, struct channel *ch);

#endif
