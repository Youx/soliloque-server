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

#ifndef __CONNECTION_PACKET_H__
#define __CONNECTION_PACKET_H__

#include <sys/socket.h>
#include <sys/types.h>
#include "server.h"

void handle_player_connect(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s);
void handle_player_keepalive(char *data, unsigned int len, struct server *s);

#endif
