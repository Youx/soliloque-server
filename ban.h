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

#ifndef __BAN_H__
#define __BAN_H__


#include <netinet/in.h>

struct ban
{
	uint16_t id;
	uint16_t duration;
	char *ip;
	char *reason;
};

struct ban *new_ban(uint16_t duration, struct in_addr ip, char *reason);
struct ban *test_ban(int x);
void destroy_ban(struct ban *b);

int ban_to_data_size(struct ban *b);
int ban_to_data(struct ban *b, char *dest);

#endif
