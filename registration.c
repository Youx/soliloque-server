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

#include "registration.h"
#include "log.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Allocate and return a new registration structure
 * used to check credentials on login.
 *
 * @return the allocated structure
 */
struct registration *new_registration()
{
	struct registration *r;

	r = (struct registration *)calloc(1, sizeof(struct registration));
	if (r == NULL) {
		logger(LOG_WARN, "new_registration, calloc failed : %s.", strerror(errno));
		return NULL;
	}

	return r;
}
