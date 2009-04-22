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

#include "player_stat.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/**
 * Allocate a new player stat structure
 */
struct player_stat *new_plstat()
{
	struct player_stat *ps;

	ps = (struct player_stat *)calloc(1, sizeof(struct player_stat));
	if (ps == NULL) {
		printf("(WW) new_plstat, calloc failed : %s.\n", strerror(errno));
		return NULL;
	}
	return ps;
}
