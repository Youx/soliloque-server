/*
 * soliloque-server, an open source implementation of the TeamSpeak protocol.
 * Copyright (C) 2009 Hugo Camboulive <hugo.camboulive AT gmail.com>
 *
 * strndup :
 * Copyright (C) 1996, 1997, 1998, 2001, 2002, 2003, 2005, 2006, 2007
 * Free Software Foundation, Inc.
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

#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "compat.h"
#include "log.h"

#ifndef HAVE_STRNDUP

char *strndup (char const *s, size_t n)
{
	size_t i;
	char *res;

	for (i = 0 ; i < n ; i++) {
		if (s[i] == '\0')
			break;
	}
	res = calloc(i + 1, sizeof(char));
	if (res == NULL) {
		logger(LOG_ERR, "strndup, calloc failed : %s.", strerror(errno));
		return NULL;
	}
	return memcpy(res, s, i);
}
#endif /* HAVE_STRNDUP */

#include <stddef.h>

char *ustrtohex (unsigned char *data, size_t len)
{
	char *dst;
	size_t i;

	dst = calloc(2 * len + 1, sizeof(char));

	for (i = 0 ; i < len ; i++) {
		sprintf(dst + (i * 2), "%02x", data[i]);
	}

	return dst;
}

void wu64(uint64_t val, char **ptr)
{
	*(uint64_t *)(*ptr) = GUINT64_TO_LE(val);
	*ptr += 8;
}

void wu32(uint32_t val, char **ptr)
{
	*(uint32_t *)(*ptr) = GUINT32_TO_LE(val);
	*ptr += 4;
}

void wu16(uint16_t val, char **ptr)
{
	*(uint16_t *)(*ptr) = GUINT16_TO_LE(val);
	*ptr += 2;
}

void wu8(uint8_t val, char **ptr)
{
	*(uint8_t *)(*ptr) = val;
	*ptr += 1;
}

void wstaticstring(char *str, int maxlen, char **ptr)
{
	char len;
	len = MIN(maxlen, strlen(str));
	**ptr = len;
	(*ptr) += 1;
	strncpy(*ptr, str, len);
	(*ptr) += maxlen;
}

uint32_t ru32(char **ptr)
{
	*ptr += 4;
	return GUINT32_FROM_LE(*(uint32_t *)(*ptr - 4));
}

uint64_t ru64(char **ptr)
{
	*ptr += 8;
	return GUINT64_FROM_LE(*(uint64_t *)(*ptr - 8));
}

uint16_t ru16(char **ptr)
{
	*ptr += 2;
	return GUINT16_FROM_LE(*(uint16_t *)(*ptr - 2));
}

uint8_t ru8(char **ptr)
{
	*ptr += 1;
	return *(uint8_t *)(*ptr - 1);
}

char *rstaticstring(int maxlen, char **ptr)
{
	int len = MIN(maxlen, **ptr);
	char *res = strndup((*ptr) + 1, len);
	*ptr += (1 + maxlen);
	return res;
}
