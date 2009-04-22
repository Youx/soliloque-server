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

#ifndef __PLAYER_STAT_H__
#define __PLAYER_STAT_H__

#include <stdint.h>
#include <time.h>

struct player_stat
{
	uint16_t version[4];

	time_t start_time; /* time of connection */

	int ping; /* test ping */
	time_t activ_time; /* time of last activity (=> idle)*/
	/* to determine the proportion of list packets */
	int pkt_rec;
	int pkt_sent;
	int size_rec;
	int size_sent;
	int pkt_lost;
	/* ip adress */
	char *ip;
	
	int bytes_send;
	int bytes_received;
};

struct player_stat *new_plstat(void);

#endif
