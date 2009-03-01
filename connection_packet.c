/**
 *  connection_packet.c
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Hugo Camboulive
 *
 */

#include "connection_packet.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "packet_tools.h"
#include "server.h"
#include "player.h"
#include "control_packet.h"
#include "server_stat.h"
#include "registration.h"
#include "server_privileges.h"


/**
 * Send a packet to the player, indicating that his connection
 * was accepted.
 *
 * @param pl the player we send this packet to
 */
static void server_accept_connection(struct player *pl)
{
	char *data, *ptr;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(436, sizeof(char));
	if (data == NULL) {
		printf("(WW) server_accept_connection : calloc failed : %s.\n", strerror(errno));
		return;
	}
	ptr = data;

	*(uint32_t *)ptr = 0x0004bef4;			ptr += 4;	/* Function field */
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* Private ID */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* Public ID */
	*(uint32_t *)ptr = pl->f4_s_counter;		ptr += 4;	/* Packet counter */
	/* Checksum initialize at the end */		ptr += 4;
	*ptr = MIN(29, strlen(s->server_name));		ptr += 1;	/* Length of server name */
	strncpy(ptr, s->server_name, *(ptr - 1));	ptr += 29;	/* Server name */
	*ptr = MIN(29, strlen(s->machine));			ptr += 1;	/* Length of server machine */
	strncpy(ptr, s->machine, *(ptr - 1));		ptr += 29;	/* Server machine */
	/* Server version */
	*(uint16_t *)ptr = 2;				ptr += 2;	/* Server version (major 1) */
	*(uint16_t *)ptr = 0;				ptr += 2;	/* Server version (major 2) */
	*(uint16_t *)ptr = 20;				ptr += 2;	/* Server version (minor 1) */
	*(uint16_t *)ptr = 1;				ptr += 2;	/* Server version (minor 2) */
	*(uint32_t *)ptr = 1;				ptr += 4;	/* Error code (1 = OK, 2 = Server Offline */
	*(uint16_t *)ptr = 0x1FEF;			ptr += 2;	/* supported codecs (1<<codec | 1<<codec2 ...) */
							ptr += 7;
	/* 0 = SA, 1 = CA, 2 = Op, 3 = Voice, 4 = Reg, 5 = Anonymous */
	sp_to_bitfield(s->privileges, ptr);
	/* garbage data */				ptr += 71;
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* Private ID */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* Public ID */
	*ptr = MIN(255, strlen(s->welcome_msg));	ptr += 1;	/* Length of welcome message */
	strncpy(ptr, s->welcome_msg, *(ptr - 1));	ptr += 255;	/* Welcome message */

	/* Add CRC */
	packet_add_crc(data, 436, 16);
	/* Send packet */
	send_to(pl->in_chan->in_server, data, 436, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
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

	data = (char *)calloc(436, sizeof(char));
	if (data == NULL) {
		printf("(WW) server_refuse_connection : calloc failed : %s.\n", strerror(errno));
		return;
	}
	ptr = data;

	*(uint32_t *)ptr = 0x0004bef4;			ptr += 4;	/* Function field */
	/* *(uint32_t *)ptr = pl->private_id;*/		ptr += 4;	/* Private ID */
	*(uint32_t *)ptr = 5;				ptr += 4;	/* Public ID */
	*(uint32_t *)ptr = 2;				ptr += 4;	/* Packet counter */
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
	*(uint32_t *)ptr = 0xFFFFFFFA;			ptr += 4;	/* Error code (1 = OK, 2 = Server Offline, 0xFFFFFFFA = Banned */
	/* rights */					ptr += 80;

	*(uint32_t *)ptr = 0x00584430;			ptr += 4;	/* Private ID */
	*(uint32_t *)ptr = 5;				ptr += 4;	/* Public ID */
	/* *ptr = 26;*/					ptr += 1;	/* Length of welcome message */
	/* memcpy(ptr, "Bienvenue sur mon serveur.", 26);*/	ptr += 255;	/* Welcome message */

	/* Add CRC */
	packet_add_crc(data, 436, 16);
	/* Send packet */
	send_to(s, data, 436, 0, (struct sockaddr *)cli_addr, cli_len);
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
	struct player *pl;
	char password[30];
	char login[30];
	struct registration *r;

	/* Check crc */
	if (!packet_check_crc(data, len, 16))
		return;

	/* Check if the IP is banned */
	if (get_ban_by_ip(s, cli_addr->sin_addr) != NULL) {
		server_refuse_connection_ban(cli_addr, cli_len, s);
		printf("PLAYER BANNED TRIED TO CONNECT\n");
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
			printf("Invalid credentials for a registered player\n");
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

	data = (char *)calloc(24, sizeof(char));
	if (data == NULL) {
		printf("(WW) s_resp_keepalive : calloc failed : %s.\n", strerror(errno));
		return;
	}
	ptr = data;

	*(uint32_t *)ptr = 0x0002bef4;			ptr += 4;	/* Function field */
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* Private ID */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* Public ID */
	*(uint32_t *)ptr = pl->f4_s_counter;		ptr += 4;	/* Packet counter */
	/* Checksum initialize at the end */		ptr += 4;
	*(uint32_t *)ptr = ka_id;			ptr += 4;	/* ID of the keepalive to confirm */
	/* Add CRC */
	packet_add_crc(data, 24, 16);

	send_to(pl->in_chan->in_server, data, 24, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
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
	uint32_t pub_id, priv_id, ka_id;
	/* Check crc */
	if(!packet_check_crc(data, len, 16))
		return;
	/* Retrieve the player */
	priv_id = *(uint32_t *)(data + 4);
	pub_id = *(uint32_t *)(data + 8);
	pl = get_player_by_ids(s, pub_id, priv_id);
	if (pl == NULL) {
		printf("pl == NULL. Why????\n");
		return;
	}
	/* Get the counter */
	ka_id = *(uint32_t *)(data + 12);
	/* Send the keepalive response */
	s_resp_keepalive(pl, ka_id);
}
