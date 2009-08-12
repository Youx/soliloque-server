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

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "server.h"
#include <dbi/dbi.h>
#include <stdio.h>

struct config
{
	char *db_type;
	union {
		struct {
			char *path;
			char *db;
		} file;
		struct {
			char *host;
			int port;
			char *user;
			char *pass;
			char *db;
		} connection;
	} db;
	struct {
		FILE *output;
		int level;
	} log;
	dbi_conn conn;
};

void destroy_config(struct config *c);
struct config *config_parse(char *filename);

#endif
