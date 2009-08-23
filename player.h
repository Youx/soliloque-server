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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "compat.h"
#include "channel.h"
#include "configuration.h"
#include "player_stat.h"

#include <sys/types.h>
#include <sys/socket.h>

/* Channel privileges */
#define CHANNEL_PRIV_CHANADMIN  1
#define CHANNEL_PRIV_OP   	2
#define CHANNEL_PRIV_VOICE  	4
#define CHANNEL_PRIV_AUTOOP 	8
#define CHANNEL_PRIV_AUTOVOICE  16

/* Global flags */
#define GLOBAL_FLAG_UNREGISTERED 0
#define GLOBAL_FLAG_SERVERADMIN 1
#define GLOBAL_FLAG_ALLOWREG  	2
#define GLOBAL_FLAG_REGISTERED  4

/* Player attributes */
/* TODO: what are 1 and 2 ??? */
#define PL_ATTR_BLOCK_WHISPER	4
#define PL_ATTR_AWAY		8
#define PL_ATTR_MUTE_MIC	16
#define PL_ATTR_MUTE_SPK	32


struct player {
	uint32_t public_id;
	uint32_t private_id;
	
	char client[30];
	char machine[30];
	char name[30];
	char login[30];

	uint16_t version[4];
	
	uint16_t global_flags;
	uint16_t player_attributes;

	struct player_stat *stats;
	
	/* the channel the player is in */
	struct channel *in_chan;
	struct registration *reg;

	struct timeval last_ping;

	/* communication */
	struct sockaddr_in *cli_addr;
	unsigned int cli_len;

	/* packet queue */
	struct queue *packets;

	/* packet counters */
	unsigned int f0_c_counter;
	unsigned int f0_s_counter;
	uint32_t f1_c_counter;
	uint32_t f1_s_counter;
	unsigned int f4_c_counter;
	unsigned int f4_s_counter;
};

void destroy_player(struct player *p);
struct player *new_player(char *nickname, char *login, char *machine);
struct player *new_default_player(void);
struct player *new_player_from_data(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
int player_to_data(struct player *pl, char *data);
int player_to_data_size(struct player *pl);
void print_player(struct player *pl);
uint16_t player_get_channel_privileges(struct player *pl, struct channel *ch);

#endif
