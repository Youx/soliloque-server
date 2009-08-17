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
#include <string.h>
#include <time.h>

#include "log.h"
#include "configuration.h"

static struct config *c = NULL;
static char *log_header[5] = {"", "(EE)", "(WW)", "(II)", "(DBG)"};

void logger(int loglevel, char *str, ...)
{
	va_list args;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	FILE *dst = stderr;
	int lvl = LOG_INFO;
	time_t t;
	char time_fmt[26];
	char *str2 = (char *)calloc(sizeof(char), strlen(str) + 40);

	if (c != NULL) {
		lvl = c->log.level;
		dst = c->log.output;
	}

	pthread_mutex_lock(&mutex);
	va_start(args, str);
	if (loglevel <= lvl) {
		time(&t);
		ctime_r(&t, time_fmt);
		time_fmt[24] = '\0';	/* remove the trailing \n */
		sprintf(str2, "%s %s %s\n", log_header[loglevel], time_fmt, str);
		vfprintf(dst, str2, args);
		fflush(dst);
	}
	va_end(args);
	pthread_mutex_unlock(&mutex);

	free(str2);
}

void set_config(struct config *cfg)
{
	c = cfg;
}
