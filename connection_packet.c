/*
 *  connection_packet.c
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Université du Maine - IUP MIME. All rights reserved.
 *
 */

#include "connection_packet.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <assert.h>

#include "packet_tools.h"
#include "server.h"
#include "player.h"

extern struct server *ts_server;
extern int socket_desc;

static void server_accept_connection(struct player *pl)
{
	/* Packet : 
16 : header
4 : checksum
1 : size of server name
29 : server name
1 : size of server machine
29 : server machine
8 : server version (4x2)
4 : 01 00 00 00
8 : ef 1f 00 00 00 00 00 00
2 : 05 00
16 : 07 ff ff 0f fe ff fe ff 03 fe 00 00 00 00 00 e0
4 : 7f 7c 3e 00
4 : d4 00 00 00
8 : 00 00 00 6c 50 28 00 d4
8 : 00 00 00 00 00 00 00 00
00 00
94 00
04 00
00 00
4e 00 00 00
90 00
04 00 00 00
90 00
4a 00 00 00
90 00
10 00 00 00
4 : private ID
4 : public ID
1 : size of welcome message
255 : welcome message
======
436
*/
	char data[436];
	char *ptr = data;
	bzero(data, 436);
	*(uint32_t *)ptr = 0x0004bef4;     ptr += 4;
	/* private and public ID */
	*(uint32_t *)ptr = pl->private_id; ptr += 4;
	*(uint32_t *)ptr = pl->public_id;  ptr += 4;
	*(uint32_t *)ptr = 0x00000000;     ptr += 4;
	ptr +=4; /* skip checksum */
	*ptr = 14;                         ptr++;
	memcpy(ptr, "Nom du serveur", 14);     ptr += 29;
	*ptr = 18;                         ptr++;
	memcpy(ptr, "Machine du serveur", 18); ptr += 29;
	/* version serveur */
	*(uint16_t *)ptr = 2;                ptr += 2;
	*(uint16_t *)ptr = 0;                ptr += 2;
	*(uint16_t *)ptr = 20;               ptr += 2;
	*(uint16_t *)ptr = 1;                ptr += 2;
	/* error code : 1 = OK, 2 = server offline */
	*(uint32_t *)ptr = 1;                ptr += 4;
	/* garbage data */


	ptr += 80;
	/* private id & public ID */
	*(uint32_t *)ptr = pl->private_id; ptr += 4;
	*(uint32_t *)ptr = pl->public_id;  ptr += 4;
	/* welcome message */
	*ptr = 26;                         ptr++;
	memcpy(ptr, "Bienvenue sur mon serveur.", 26); ptr += 255;

	/* Add CRC */
	packet_add_crc(data, 436, 16);
	printf("ptr - data = %i\n", ptr - data);
	assert(ptr - data == 436);
	/* Send packet */
	sendto(socket_desc, data, 436, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);

}

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

