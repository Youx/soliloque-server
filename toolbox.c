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
		printf("(EE) strndup, calloc failed : %s.\n", strerror(errno));
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

	/*printf("%s\n", dst); */
	return dst;
}
