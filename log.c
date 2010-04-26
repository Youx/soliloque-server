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

#define LOG_COLOR_CANCEL "\x1b[0;37;40m"

static struct config *c = NULL;
static char *log_header[5] = {"", "(EE)", "(WW)", "(II)", "(DBG)"};
static char *log_color[5] = {"", "\x1b[0;31;40m", "\x1b[0;33;40m",
	"\x1b[0;32;40m", "\x1b[0;34;40m"};
static char *log_color_dim[5] = {"", "\x1b[2;31;40m", "\x1b[2;33;40m",
	"\x1b[2;32;40m", "\x1b[2;34;40m"};
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void logger(int loglevel, char *str, ...)
{
	va_list args;
	FILE *dst;
	int lvl;
	time_t t;
	char time_fmt[26];
	char *str2 = (char *)calloc(sizeof(char), strlen(str) + 40 + 30);

	pthread_mutex_lock(&mutex);
	if (c != NULL) {
		lvl = c->log.level;
		dst = c->log.output;
	} else {
		lvl = LOG_INFO;
		dst = stderr;
	}

	if (loglevel > 4)
		loglevel = 4;

	va_start(args, str);
	if (loglevel <= lvl) {
		time(&t);
		ctime_r(&t, time_fmt);
		time_fmt[24] = '\0';	/* remove the trailing \n */
		sprintf(str2, "%s%s %s%s"LOG_COLOR_CANCEL" %s\n", log_color[loglevel], log_header[loglevel], log_color_dim[loglevel], time_fmt, str);
		vfprintf(dst, str2, args);
		fflush(dst);
	}
	va_end(args);

	free(str2);
	pthread_mutex_unlock(&mutex);
}

void set_config(struct config *cfg)
{
	pthread_mutex_lock(&mutex);
	c = cfg;
	pthread_mutex_unlock(&mutex);
}
