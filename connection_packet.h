/*
 *  connection_packet.h
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Université du Maine - IUP MIME. All rights reserved.
 *
 */

#ifndef __CONNECTION_PACKET_H__
#define __CONNECTION_PACKET_H__

#include <sys/socket.h>
#include <sys/types.h>
#include "server.h"

void handle_player_connect(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s);
void handle_player_keepalive(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s);

#endif
