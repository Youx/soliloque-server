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

#include "player.h"
#include "player_stat.h"
#include "log.h"
#include "queue.h"
#include "player_channel_privilege.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>


/**
 * Destroys a player and free its memory
 *
 * @param p the player we want to destroy
 */
void destroy_player(struct player *p)
{
	/* not always initialized */
	if (p->cli_addr)
		free(p->cli_addr);
	if (p->stats)
		free(p->stats);
	if (p->packets)
		destroy_queue(p->packets);
	if (p->muted)
		ar_free(p->muted);
	free(p);
}

/**
 * Allocate and initialize a new player.
 * 
 * @param nickname the nickname of the player
 * @param login the login of the player
 * @param machine a description of the machine of the player (ex : uname)
 *
 * @return the initialized player
 */
struct player *new_player(char *nickname, char *login, char *machine)
{
	struct player *p;
	
	p = (struct player *)calloc(1, sizeof(struct player));
	if (p == NULL) {
		logger(LOG_WARN, "new_player, calloc failed : %s.", strerror(errno));
		return NULL;
	}
	/* create packet queue */
	p->packets = new_queue();
	p->muted = ar_new(2);
	p->stats = new_plstat();
	strcpy(p->name, nickname);
	strcpy(p->machine, machine);
	strcpy(p->client, login);
	gettimeofday(&p->last_ping, NULL);
	p->global_flags = 0;	/* remove later */
	p->f0_s_counter = 1;
	p->f0_c_counter = 1;
	p->f1_s_counter = 1;
	p->f1_c_counter = 1;
	p->f4_s_counter = 2; /* this is a mystery */
	p->f4_c_counter = 1;
	return p;
}

/**
 * Create a default player (for testing purposes)
 * 
 * @return the test player
 */
struct player *new_default_player()
{
	return new_player("Nickname", "Login", "Test Machine 0.0");
}

/**
 * Generate a player from a connection packet
 *
 * @param data the connection packet from the client
 * @param len the length of data
 * @param cli_addr the adress of the client
 * @param cli_len the length of cli_addr
 *
 * @return the allocated player
 */
struct player *new_player_from_data(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	struct player *pl;
	char *client;
	char *machine;
	char *nickname;
	char *login;
	char *password;
	size_t tmp_size;
	char *ptr = data;
	uint16_t version[4];

	/* Verify fields */
	if (len != 180) {
		logger(LOG_WARN, "new_player_from_data, packet has invalid size.");
		return NULL;
	}
	
	/* Copy fields */
	/* Bypass header */				ptr += 16;
	/* Bypass checksum */				ptr += 4;
	/* Copy the strings to a null terminated format */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of client */
	client = strndup(ptr, tmp_size);		ptr += 29;	/* client */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of machine */
	machine = strndup(ptr, tmp_size);		ptr += 29;	/* machine */
	version[0] = *(uint16_t *)ptr;			ptr += 2;
	version[1] = *(uint16_t *)ptr;			ptr += 2;
	version[2] = *(uint16_t *)ptr;			ptr += 2;
	version[3] = *(uint16_t *)ptr;			ptr += 2;
	/* not used yet */				ptr += 2;
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of login */
	login = strndup(ptr, tmp_size);			ptr += 29;	/* login */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of password */
	password = strndup(ptr, tmp_size);		ptr += 29;	/* password */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of nickname */
	nickname = strndup(ptr, tmp_size);		ptr += 29;	/* nickname */

	/* check we filled the whole data */
	assert(ptr - data == len);
	
	/* Initialize player */
	pl = new_player(nickname, login, machine);
	pl->version[0] = version[0];
	pl->version[1] = version[1];
	pl->version[2] = version[2];
	pl->version[3] = version[3];
	pl->stats->start_time = time(NULL);
	pl->stats->activ_time = time(NULL);
	/* Alloc adresses */
	pl->cli_addr = (struct sockaddr_in *)calloc(cli_len, sizeof(char));
	if (pl->cli_addr == NULL) {
		logger(LOG_WARN, "new_player_from_data, client address calloc failed : %s.", strerror(errno));
		destroy_player(pl);
		return NULL;
	}
	memcpy(pl->cli_addr, cli_addr, cli_len);
	pl->cli_len = cli_len;

	logger(LOG_INFO, "machine : %s, login : %s, nickname : %s", pl->machine, pl->client, pl->name);
	free(client); free(machine); free(nickname); free(login); free(password);
	return pl;
}

/**
 * Converts a player to raw data to be transmitted
 *
 * @param pl the player to convert
 * @param data the data to write into (already allocated)
 *
 * @return the number of bytes written into data
 */
int player_to_data(struct player *pl, char *data)
{
	int size = player_to_data_size(pl);
	char *ptr = data;

	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* public ID */
	*(uint32_t *)ptr = pl->in_chan->id;		ptr += 4;	/* channel ID */
	*(uint16_t *)ptr = player_get_channel_privileges(pl, pl->in_chan);	ptr += 2;	/* player chan privileges */
	*(uint16_t *)ptr = pl->global_flags;		ptr += 2;	/* player global flags */
	*(uint16_t *)ptr = pl->player_attributes;	ptr += 2;	/* player attributes */
	*ptr = MIN(29, strlen(pl->name));		ptr += 1;	/* player name size */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;	/* player name */

	return size;
}

/**
 * Compute the number of bytes a player is going to take
 * once converted to raw data.
 *
 * @param pl the player
 *
 * @return the number of bytes that will be needed
 */
int player_to_data_size(struct player *pl)
{
	return 4 + 4 + 2 + 2 + 2 + 1 + 29; /* 44 */
}

void print_player(struct player *pl)
{
	logger(LOG_INFO, "Player : %s", pl->name);
	logger(LOG_INFO, "\tpublic ID  : 0x%x", pl->public_id);
	logger(LOG_INFO, "\tprivate ID : 0x%x", pl->private_id);
	logger(LOG_INFO, "\tmachine    : %s", pl->machine);
	logger(LOG_INFO, "\tclient     : %s", pl->client);
}

/**
 * For a given player and channel, return the corresponding
 * channel privileges as a bit field
 *
 * @param pl the player
 * @param ch the channel
 *
 * @return the channel privileges bitfield
 */
uint16_t player_get_channel_privileges(struct player *pl, struct channel *ch)
{
	uint16_t res = 0;
	size_t iter;
	struct player_channel_privilege *tmp_priv;
	struct channel *tmp_ch;

	/* if this is a subchannel, look in the parent channel */
	tmp_ch = ch;
	if (ch->parent != NULL)
		tmp_ch = ch->parent;

	/* look in the current channel */
	ar_each(struct player_channel_privilege *, tmp_priv, iter, tmp_ch->pl_privileges)
		if (tmp_priv->reg == PL_CH_PRIV_REGISTERED && tmp_priv->pl_or_reg.reg == pl->reg)
			res = tmp_priv->flags;
		else if (tmp_priv->reg == PL_CH_PRIV_UNREGISTERED && tmp_priv->pl_or_reg.pl == pl)
			res = tmp_priv->flags;
	ar_end_each;

	return res;
}
