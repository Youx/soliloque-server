#include "configuration.h"

#include <libconfig.h>
#include <string.h>
#include <stdlib.h>

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
	config_setting_t *curr;
	struct config *cfg_s;

	config_init(&cfg);
	if (config_read_file(&cfg, cfg_file) == CONFIG_FALSE) {
		printf("Error loading file %s\n", cfg_file);
		return 0;
	}

	db = config_lookup(&cfg, "db");
	if (db == NULL) {
		printf("Error : no db tag.\n");
		return 0;
	} else {
		cfg_s = (struct config *)calloc(1, sizeof(struct config));
	}
	/* get the database type */
	curr = config_setting_get_member(db, "type");
	if (curr == NULL) /* default : sqlite3 */
		cfg_s->db_type = "sqlite3";
	else
		cfg_s->db_type = strdup(config_setting_get_string(curr));

	if (strcmp(cfg_s->db_type, "sqlite") == 0 || strcmp(cfg_s->db_type, "sqlite3") == 0) {
		/* sqlite 2.x or 3.x use a filename to connect */
		curr = config_setting_get_member(db, "dir");
		if (curr == NULL)
			cfg_s->db.file.path = "./";
		else
			cfg_s->db.file.path = strdup(config_setting_get_string(curr));

		curr = config_setting_get_member(db, "db");
		if (curr == NULL)
			cfg_s->db.file.db = "soliloque.sqlite3";
		else
			cfg_s->db.file.db = strdup(config_setting_get_string(curr));
	} else {
		/* anything else uses a host, port, user, pass, db */

		/* get the hostname */
		curr = config_setting_get_member(db, "host");
		if (curr == NULL)
			cfg_s->db.connection.host = "localhost";
		else
			cfg_s->db.connection.host = strdup(config_setting_get_string(curr));
		/* get the port */
		curr = config_setting_get_member(db, "port");
		if (curr == NULL)
			cfg_s->db.connection.port = 3306;
		else
			cfg_s->db.connection.port = config_setting_get_int(curr);
		/* get the username */
		curr = config_setting_get_member(db, "user");
		if (curr == NULL)
			cfg_s->db.connection.user = "root";
		else
			cfg_s->db.connection.user = strdup(config_setting_get_string(curr));
		/* get the password */
		curr = config_setting_get_member(db, "pass");
		if (curr == NULL)
			cfg_s->db.connection.pass = "";
		else
			cfg_s->db.connection.pass = strdup(config_setting_get_string(curr));
		/* get the database */
		curr = config_setting_get_member(db, "db");
		if (curr == NULL)
			cfg_s->db.connection.db = "soliloque";
		else
			cfg_s->db.connection.db = strdup(config_setting_get_string(curr));
	}

	return cfg_s;
}

