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

#include "connection_packet.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include "packet_tools.h"
#include "server.h"
#include "player.h"
#include "control_packet.h"
#include "server_stat.h"
#include "registration.h"
#include "server_privileges.h"
#include "log.h"


/**
 * Send a packet to the player, indicating that his connection
 * was accepted.
 *
 * @param pl the player we send this packet to
 */
static void server_accept_connection(struct player *pl)
{
	char *data, *ptr;
	int data_size = 436;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_ERR, "server_accept_connection : calloc failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu32(0x0004bef4, &ptr);		/* Function field */
	wu32(pl->private_id, &ptr);	/* Private ID */
	wu32(pl->public_id, &ptr);	/* Public ID */
	wu32(pl->f4_s_counter, &ptr);	/* Packet counter */
	ptr += 4;			/* Checksum initialize at the end */	
	
	wstaticstring(s->server_name, 29, &ptr);/* Server name */
	wstaticstring(s->machine, 29, &ptr);	/* Server machine */

	/* Server version */
	wu16(2, &ptr);			/* Server version (major 1) */
	wu16(0, &ptr);			/* Server version (major 2) */
	wu16(20, &ptr);			/* Server version (minor 1) */
	wu16(1, &ptr);			/* Server version (minor 2) */
	wu32(1, &ptr);			/* Error code (1 = OK, 2 = Server Offline */
	wu16(0x1FEF, &ptr);		/* supported codecs (1<<codec | 1<<codec2 ...) */

	ptr += 7;
	/* 0 = SA, 1 = CA, 2 = Op, 3 = Voice, 4 = Reg, 5 = Anonymous */
	sp_to_bitfield(s->privileges, ptr);
	/* garbage data */
	ptr += 71;
	wu32(pl->private_id, &ptr);	/* Private ID */
	wu32(pl->public_id, &ptr);	/* Public ID */

	wstaticstring(s->welcome_msg, 255, &ptr);	/* Welcome message */

	/* check we filled the whole packet */
	assert((ptr - data) == data_size);

	/* Add CRC */
	packet_add_crc(data, 436, 16);
	/* Send packet */
	/*send_to(pl->in_chan->in_server, data, 436, 0, pl);*/
	sendto(pl->in_chan->in_server->socket_desc, data, 436, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f4_s_counter++;
	free(data);
}

/**
 * Refuse a connection from a player because he has been banned.
 *
 * @param cli_addr the address of the player
 * @param cli_len the length of cli_addr
 */
static void server_refuse_connection_ban(struct sockaddr_in *cli_addr, int cli_len, struct server *s)
{
	char *data, *ptr;
	int data_size = 436;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_ERR, "server_refuse_connection : calloc failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu32(0x0004bef4, &ptr);		/* Function field */
	/* *(uint32_t *)ptr = pl->private_id;*/		ptr += 4;	/* Private ID */
	wu32(5, &ptr);			/* Public ID */
	wu32(2, &ptr);			/* Packet counter */
	/* Checksum initialize at the end */		ptr += 4;
	/* *ptr = 14;*/					ptr += 1;	/* Length of server name */
	/* memcpy(ptr, "Nom du serveur", 14);*/		ptr += 29;	/* Server name */
	/* *ptr = 18;*/					ptr += 1;	/* Length of server machine */
	/* memcpy(ptr, "Machine du serveur", 18);*/	ptr += 29;	/* Server machine */
	/* Server version */
	/* *(uint16_t *)ptr = 2;*/			ptr += 2;	/* Server version (major 1) */
	/* *(uint16_t *)ptr = 0;*/			ptr += 2;	/* Server version (major 2) */
	/* *(uint16_t *)ptr = 20;*/			ptr += 2;	/* Server version (minor 1) */
	/* *(uint16_t *)ptr = 1;*/			ptr += 2;	/* Server version (minor 2) */
	wu32(0xFFFFFFFA, &ptr);	/* Error code (1 = OK, 2 = Server Offline, 0xFFFFFFFA = Banned */
	/* rights */					ptr += 80;

	wu32(0x00584430, &ptr);	/* Private ID */
	wu32(5, &ptr);		/* Public ID */
	/* *ptr = 26;*/					ptr += 1;	/* Length of welcome message */
	/* memcpy(ptr, "Bienvenue sur mon serveur.", 26);*/	ptr += 255;	/* Welcome message */

	/* check we filled the whole packet */
	assert((ptr - data) == data_size);

	/* Add CRC */
	packet_add_crc(data, 436, 16);
	/* Send packet */
	sendto(s->socket_desc, data, 436, 0, (struct sockaddr *)cli_addr, cli_len);
	free(data);
}

/**
 * Handle a connection attempt from a player :
 * - check the crc
 * - check server/user password (TODO)
 * - check if the player is already connected (TODO)
 * - initialize the player, add it to the pool
 * - notify the player if he has been accepted (or not TODO)
 * - notify the other players (TODO)
 * - move the player to the default channel (TODO)
 *
 * @param data the connection packet
 * @param len the length of the connection packet
 * @param cli_addr the adress of the client
 * @param cli_len the length of cli_addr
 */
void handle_player_connect(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	struct player *pl, *tmp_pl;
	char password[30];
	char login[30];
	struct registration *r;
	size_t iter;

	/* Check crc */
	if (!packet_check_crc(data, len, 16))
		return;

	/* Check if the IP is banned */
	if (get_ban_by_ip(s, cli_addr->sin_addr) != NULL) {
		server_refuse_connection_ban(cli_addr, cli_len, s);
		logger(LOG_INFO, "Banned player tried to connect");
		return;
	}
	/* If registered, check if player exists, else check server password */
	bzero(password, 30 * sizeof(char));
	strncpy(password, data + 121, MIN(29, data[120]));
	bzero(login, 30 * sizeof(char));
	strncpy(login, data + 91, MIN(29, data[90]));
 
	pl = new_player_from_data(data, len, cli_addr, cli_len);
	if (data[90] == 0) {	/* no login = anonymous mode */
		/* check password against server password */
		if (strcmp(password, s->password) != 0) {
			destroy_player(pl);
			return;	/* wrong server password */
		}
		pl->global_flags = GLOBAL_FLAG_UNREGISTERED;
	} else {
		r = get_registration(s, login, password);
		if (r == NULL) {
			logger(LOG_INFO, "Invalid credentials for a registered player (%s)", login);
			destroy_player(pl);
			return;	/* nobody found with those credentials */
		}
		pl->global_flags |= r->global_flags;
		pl->global_flags |= GLOBAL_FLAG_REGISTERED;
		pl->reg = r;
	}

	/* Add player to the pool */
	add_player(s, pl);
	/* Send a message to the client indicating he has been accepted */

	/* Send server information to the player (0xf4be0400) */
	server_accept_connection(pl);
	/* Send a message to all players saying that a new player arrived (0xf0be6400) */
	s_notify_new_player(pl);
	/* Send the new player the list of all the Voice Requests */
	ar_each(struct player *, tmp_pl, iter, s->players)
		if (pl->player_attributes & PL_ATTR_REQUEST_VOICE)
			s_notify_player_requested_voice(tmp_pl, pl);
	ar_end_each;
}

/**
 * Send a keepalive back to the client to confirm we have received his
 *
 * @param s the server
 * @param pl the player to send this to
 * @param ka_id the counter of the keepalived we received
 */
static void s_resp_keepalive(struct player *pl, uint32_t ka_id)
{
	char *data, *ptr;
	int data_size = 24;

	data = (char *)calloc(data_size, sizeof(char));
	if (data == NULL) {
		logger(LOG_ERR, "s_resp_keepalive : calloc failed : %s.", strerror(errno));
		return;
	}
	ptr = data;

	wu32(0x0002bef4, &ptr);		/* Function field */
	wu32(pl->private_id, &ptr);	/* Private ID */
	wu32(pl->public_id, &ptr);	/* Public ID */
	wu32(pl->f4_s_counter, &ptr);	/* Packet counter */
	/* Checksum initialize at the end */		ptr += 4;
	wu32(ka_id, &ptr);		/* ID of the keepalive to confirm */

	/* check we filled the whole packet */
	assert((ptr - data) == data_size);

	/* Add CRC */
	packet_add_crc(data, 24, 16);

	sendto(pl->in_chan->in_server->socket_desc, data, 24, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f4_s_counter++;
	free(data);
}

/**
 * Handle a keepalive sent by the client
 * - check the crc
 * - send a keepalive back to the client
 *
 * @param data the connection packet
 * @param len the length of the connection packet
 * @param cli_addr the adress of the client
 * @param cli_len the length of cli_addr
 */
void handle_player_keepalive(char *data, unsigned int len, struct server *s)
{
	struct player *pl;
	char *ptr = data;
	uint32_t pub_id, priv_id, ka_id;
	/* Check crc */
	if(!packet_check_crc(data, len, 16))
		return;
	/* Retrieve the player */
	ptr += 4;
	priv_id = ru32(&ptr);
	pub_id = ru32(&ptr);
	ka_id = ru32(&ptr); 	/* Get the counter */
	pl = get_player_by_ids(s, pub_id, priv_id);
	if (pl == NULL) {
		logger(LOG_WARN, "handle_player_keepalive : pl == NULL. Why????");
		return;
	}
	/* Send the keepalive response */
	s_resp_keepalive(pl, ka_id);
	/* Update the last_ping field */
	gettimeofday(&pl->last_ping, NULL);
}
