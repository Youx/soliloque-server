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

#include "database.h"
#include "log.h"

#include <string.h>
#include <dbi/dbi.h>

/**
 * Initialize the database from a configuration
 *
 * @param c the config of the db
 *
 * @return 1 on success
 */
int init_db(struct config *c)
{
	dbi_initialize(NULL);
	c->conn = dbi_conn_new(c->db_type);

	if (strcmp(c->db_type, "sqlite") == 0 || strcmp(c->db_type, "sqlite3") == 0) {
		if (strcmp(c->db_type, "sqlite") == 0)
			dbi_conn_set_option(c->conn, "sqlite_dbdir", c->db.file.path);
		else
			dbi_conn_set_option(c->conn, "sqlite3_dbdir", c->db.file.path);

		dbi_conn_set_option(c->conn, "dbname", c->db.file.db);
	} else {
		dbi_conn_set_option(c->conn, "host", c->db.connection.host);
		dbi_conn_set_option(c->conn, "username", c->db.connection.user);
		dbi_conn_set_option(c->conn, "password", c->db.connection.pass);
		dbi_conn_set_option(c->conn, "dbname", c->db.connection.db);
		dbi_conn_set_option_numeric(c->conn, "port", c->db.connection.port);
	}

	return 1;
}

/**
 * Connect to the database before executing a query
 *
 * @param c the configuration of the db
 *
 * @return 0 on failure, 1 on success
 */
int connect_db(struct config *c)
{
	INFO("Connecting to db");
	if (dbi_conn_connect(c->conn) < 0) {
		ERROR("Could not connect. Please check the option settings");
		return 0;
	}
	return 1;
}
