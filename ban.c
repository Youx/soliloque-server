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

#include "ban.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

/**
 * Create and initialize a new ban.
 * The ID is assigned only when the ban is added to the server.
 *
 * @param duration the duration of the ban (0 = unlimited)
 * @param ip the ip of the banned player
 * @param reason the reason/description of the ban
 *
 * @return the new ban
 */
struct ban *new_ban(uint16_t duration, struct in_addr ip, char *reason)
{
	struct ban *b = (struct ban *)calloc(1, sizeof(struct ban));

	if (b == NULL) {
		logger(LOG_ERR, "new_ban, calloc failed : %s.", strerror(errno));
		return NULL;
	}
	
	b->duration = duration;
	b->ip = strdup(inet_ntoa(ip));
	b->reason = strdup(reason);
	if (b->ip == NULL || b->reason == NULL) {
		if (b->ip != NULL)
			free(b->ip);
		if (b->reason != NULL)
			free(b->reason);
		free(b);
		return NULL;
	}

	return b;
}

/**
 * Generate a testing ban
 *
 * @param x 0 or 1 to get different values
 *
 * @return the created ban
 */
struct ban *test_ban(int x)
{
	struct ban *b = (struct ban *)calloc(1, sizeof(struct ban));

	if (b == NULL) {
		logger(LOG_ERR, "test_ban, calloc failed : %s.", strerror(errno));
		return NULL;
	}

	if (x == 0) {
		b->duration = 0;
		b->ip = "192.168.100.100";
		b->reason = "CBIENFAIT";
	} else {
		b->duration = 200;
		b->ip = "192.168.1.1";
		b->reason = "CBIENFAIT caca culotte blablablablabla";
	}
	return b;
}

/**
 * Return the size a ban will take once converted to
 * raw data (to be transmitted).
 *
 * @param b the ban
 *
 * @return the number of bytes it will take
 */
int ban_to_data_size(struct ban *b)
{
	/* IP (*) + Duration (2) + reason (*) */
	return (strlen(b->ip) +1) + 2 + (strlen(b->reason) +1);
}

/**
 * Convert a ban to raw data and put it into dest.
 *
 * @param b the ban
 * @param dest the destination buffer (already allocated)
 *
 * @return the number of bytes written
 */
int ban_to_data(struct ban *b, char *dest)
{
	char *ptr;

	ptr = dest;
	strcpy(ptr, b->ip);		ptr += strlen(b->ip) + 1;	/* ip */
	*(uint16_t *)ptr = b->duration;	ptr += 2;			/* duration in minutes */
	strcpy(ptr, b->reason);		ptr += strlen(b->reason) + 1;	/* reason */

	return ptr - dest;
}
