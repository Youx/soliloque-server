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

#include "configuration.h"
#include "log.h"

#include <libconfig.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "server.h"

/**
 * Destroy an allocated configuration structure and all
 * of its fields that are not null.
 *
 * @param c the configuration structure
 */
void destroy_config(struct config *c)
{
	if (c->db_type != NULL) {
		if (strcmp(c->db_type, "sqlite") == 0 || strcmp(c->db_type, "sqlite3") == 0) {
			if (c->db.file.path != NULL)
				free(c->db.file.path);
			if (c->db.file.db != NULL)
				free(c->db.file.db);
		} else {
			if (c->db.connection.host != NULL)
				free(c->db.connection.host);
			if (c->db.connection.pass != NULL)
				free(c->db.connection.pass);
			if (c->db.connection.user != NULL)
				free(c->db.connection.user);
		}
		free(c->db_type);
	}
	free(c);
}

static int config_parse_log(config_setting_t *log, struct config *cfg)
{
	config_setting_t *curr;
	char *file;

	curr = config_setting_get_member(log, "output");
	if (curr == NULL) {
		cfg->log.output = stderr;
	} else {
		file = strdup(config_setting_get_string(curr));
		if (strcmp(file, "stderr") == 0) {
			cfg->log.output = stderr;
		} else if (strcmp(file, "stdout") == 0) {
			cfg->log.output = stdout;
		} else {
			cfg->log.output = fopen(file, "a");
			if (cfg->log.output == NULL) {
				cfg->log.output = stderr;
				logger(LOG_WARN, "config_parse_log : could not open file %s (%s)", file, strerror(errno));
			}
		}
	}

	curr = config_setting_get_member(log, "level");
	if (curr == NULL)
		cfg->log.level = 3;
	else
		cfg->log.level = config_setting_get_int(curr);
	return 1;
}

static int config_parse_db_sqlite(config_setting_t *db, struct config *cfg)
{
	config_setting_t *curr;

	/* sqlite 2.x or 3.x use a filename to connect */
	curr = config_setting_get_member(db, "dir");
	if (curr == NULL)
		cfg->db.file.path = "./";
	else
		cfg->db.file.path = strdup(config_setting_get_string(curr));

	curr = config_setting_get_member(db, "db");
	if (curr == NULL)
		cfg->db.file.db = "soliloque.sqlite3";
	else
		cfg->db.file.db = strdup(config_setting_get_string(curr));

	return 1;
}

static int config_parse_db_other(config_setting_t *db, struct config *cfg)
{
	config_setting_t *curr;

	/* anything else uses a host, port, user, pass, db */

	/* get the hostname */
	curr = config_setting_get_member(db, "host");
	if (curr == NULL)
		cfg->db.connection.host = "localhost";
	else
		cfg->db.connection.host = strdup(config_setting_get_string(curr));
	/* get the port */
	curr = config_setting_get_member(db, "port");
	if (curr == NULL)
		cfg->db.connection.port = 3306;
	else
		cfg->db.connection.port = config_setting_get_int(curr);
	/* get the username */
	curr = config_setting_get_member(db, "user");
	if (curr == NULL)
		cfg->db.connection.user = "root";
	else
		cfg->db.connection.user = strdup(config_setting_get_string(curr));
	/* get the password */
	curr = config_setting_get_member(db, "pass");
	if (curr == NULL)
		cfg->db.connection.pass = "";
	else
		cfg->db.connection.pass = strdup(config_setting_get_string(curr));
	/* get the database */
	curr = config_setting_get_member(db, "db");
	if (curr == NULL)
		cfg->db.connection.db = "soliloque";
	else
		cfg->db.connection.db = strdup(config_setting_get_string(curr));

	return 1;
}

static int config_parse_db(config_setting_t *db, struct config *cfg)
{
	config_setting_t *curr;

	/* get the database type */
	curr = config_setting_get_member(db, "type");
	if (curr == NULL) /* default : sqlite3 */
		cfg->db_type = "sqlite3";
	else
		cfg->db_type = strdup(config_setting_get_string(curr));

	if (strcmp(cfg->db_type, "sqlite") == 0 || strcmp(cfg->db_type, "sqlite3") == 0)
		return config_parse_db_sqlite(db, cfg);
	else
		return config_parse_db_other(db, cfg);
}

/**
 * Parse a file and generate a configuration structure
 *
 * @param cfg_file the input filename
 *
 * @return the allocated structure
 */
struct config *config_parse(char *cfg_file)
{
	config_t cfg;
	config_setting_t *db;
	config_setting_t *log;
	config_setting_t *curr;
	struct config *cfg_s;

	config_init(&cfg);
	if (config_read_file(&cfg, cfg_file) == CONFIG_FALSE) {
		logger(LOG_ERR, "Error loading file %s", cfg_file);
		config_destroy(&cfg);
		return 0;
	}

	db = config_lookup(&cfg, "db");
	if (db == NULL) {
		logger(LOG_ERR, "Error : no db tag.");
		config_destroy(&cfg);
		return 0;
	}
	cfg_s = (struct config *)calloc(1, sizeof(struct config));
	if (cfg_s == NULL) {
		logger(LOG_ERR, "config_parse, calloc failed : %s.", strerror(errno));
		config_destroy(&cfg);
		return 0;
	}
	if (config_parse_db(db, cfg_s) == 0) {
		logger(LOG_ERR, "config_parse_db failed.");
		config_destroy(&cfg);
		return 0;
	}

	log = config_lookup(&cfg, "log");
	if (db == NULL) {
		logger(LOG_ERR, "Error : no log tag.");
		config_destroy(&cfg);
		return 0;
	}
	if (config_parse_log(log, cfg_s) == 0) {
		logger(LOG_ERR, "config_parse_log failed.");
		config_destroy(&cfg);
		return 0;
	}

	config_destroy(&cfg);
	return cfg_s;
}

