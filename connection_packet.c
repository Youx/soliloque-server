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
#include <resolv.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "packet_tools.h"
#include "server.h"
#include "player.h"

extern struct server *ts_server;
extern int socket_desc;

/**
 * Send a packet to the player, indicating that his connection
 * was accepted.
 *
 * @param pl the player we send this packet to
 */
static void server_accept_connection(struct player *pl)
{
	char *data = (char *)calloc(436, sizeof(char));
	char *ptr = data;
	*(uint32_t *)ptr = 0x0004bef4;			ptr+=4;	/* Function field */
	*(uint32_t *)ptr = pl->private_id;		ptr+=4;	/* Private ID */
	*(uint32_t *)ptr = pl->public_id;		ptr+=4;	/* Public ID */
	*(uint32_t *)ptr = pl->f4_s_counter;		ptr+=4;	/* Packet counter */
	/* Checksum initialize at the end */		ptr+=4;
	*ptr = 14;					ptr++;		/* Length of server name */
	memcpy(ptr, "Nom du serveur", 14);		ptr+=29;	/* Server name */
	*ptr = 18;					ptr++;		/* Length of server machine */
	memcpy(ptr, "Machine du serveur", 18);		ptr+=29;	/* Server machine */
	/* Server version */
	*(uint16_t *)ptr = 2;				ptr+=2;	/* Server version (major 1) */
	*(uint16_t *)ptr = 0;				ptr+=2;	/* Server version (major 2) */
	*(uint16_t *)ptr = 20;				ptr+=2;	/* Server version (minor 1) */
	*(uint16_t *)ptr = 1;				ptr+=2;	/* Server version (minor 2) */
	*(uint32_t *)ptr = 1;				ptr+=4;	/* Error code (1 = OK, 2 = Server Offline */
	/* garbage data */				ptr+=80;
	*(uint32_t *)ptr = pl->private_id;		ptr+=4;	/* Private ID */
	*(uint32_t *)ptr = pl->public_id;		ptr+=4;	/* Public ID */
	*ptr = 26;					ptr++;		/* Length of welcome message */
	memcpy(ptr, "Bienvenue sur mon serveur.", 26); ptr += 255;	/* Welcome message */

	/* Add CRC */
	packet_add_crc(data, 436, 16);
	/* Send packet */
	sendto(socket_desc, data, 436, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f4_s_counter++;
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
void handle_player_connect(char *data, unsigned int len, struct sockaddr_in * cli_addr, unsigned int cli_len)
{
	struct player *pl;
	/* Check crc */
	if(!packet_check_crc(data, len, 16))
		return;
	/* If registered, check if player exists, else check server password */


	/* Add player to the pool */
	pl = new_player_from_data(data, len, cli_addr, cli_len);
	add_player(ts_server, pl);
	/* Send a message to the client indicating he has been accepted */

	/* Send server information to the player (0xf4be0400) */
	server_accept_connection(pl);
	/* Send a message to all players saying that a new player arrived (0xf0be6400) */
}

