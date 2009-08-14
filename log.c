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

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#include "log.h"
#include "configuration.h"

static struct config *c = NULL;

void logger(int loglevel, char *str, ...)
{
	va_list args;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	FILE *dst = stderr;
	int lvl = LOG_INFO;

	if (c != NULL) {
		lvl = c->log.level;
		dst = c->log.output;
	}

	pthread_mutex_lock(&mutex);
	va_start(args, str);
	if (loglevel <= lvl) {
		vfprintf(dst, str, args);
		fflush(dst);
	}
	va_end(args);
	pthread_mutex_unlock(&mutex);
}

void set_config(struct config *cfg)
{
	c = cfg;
}
