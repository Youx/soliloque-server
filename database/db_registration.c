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
#include "registration.h"

#include <string.h>
#include <dbi/dbi.h>

/**
 * Go through the database, read and add to the server all
 * the registrations stored.
 *
 * @param c the configuration of the db
 * @param s the server
 */
int db_create_registrations(struct config *c, struct server *s)
{
	char *q = "SELECT * FROM registrations WHERE server_id = %i;";
	struct registration *r;
	char *name, *pass;
	dbi_result res;

	res = dbi_conn_queryf(c->conn, q, s->id);

	if (res) {
		while (dbi_result_next_row(res)) {
			r = new_registration();
			r->db_id = dbi_result_get_uint(res, "id");
			r->global_flags = dbi_result_get_uint(res, "serveradmin");
			name = dbi_result_get_string_copy(res, "name");
			strncpy(r->name, name, MIN(29, strlen(name)));
			pass = dbi_result_get_string_copy(res, "password");
			strcpy(r->password, pass);
			add_registration(s, r);
			/* free temporary variables */
			free(pass); free(name);
		}
		dbi_result_free(res);
	}
	return 1;
}

/**
 * Add a new registration to the database
 *
 * @param c the db configuration
 * @param s the server
 * @param r the registration
 *
 * @return 1 on success
 */
int db_add_registration(struct config *c, struct server *s, struct registration *r)
{
	char *req = "INSERT INTO registrations (server_id, serveradmin, name, password) VALUES (%i, %i, %s, %s);";
	char *quoted_name, *quoted_pass;
	dbi_result res;
	struct channel *ch;
	struct player_channel_privilege *priv;
	size_t iter, iter2;

	dbi_conn_quote_string_copy(c->conn, r->name, &quoted_name);
	dbi_conn_quote_string_copy(c->conn, r->password, &quoted_pass);

	res = dbi_conn_queryf(c->conn, req, s->id, r->global_flags, quoted_name, quoted_pass);
	if (res == NULL) {
		logger(LOG_WARN, "db_add_registration : SQL query failed");
	} else {
		r->db_id = dbi_conn_sequence_last(c->conn, NULL);
		dbi_result_free(res);
	}
	free(quoted_pass);
	free(quoted_name);

	ar_each(struct channel *, ch, iter, s->chans)
		ar_each(struct player_channel_privilege *, priv, iter2, ch->pl_privileges)
			if (priv->reg == PL_CH_PRIV_REGISTERED && priv->pl_or_reg.reg == r) {
				logger(LOG_INFO, "db_add_registration : adding a new pl_chan_priv");
				db_add_pl_chan_priv(c, priv);
			}
		ar_end_each;
	ar_end_each;
	return 1;
}

int db_del_registration(struct config *c, struct server *s, struct registration *r)
{
	char *q = "DELETE FROM registrations WHERE id = %i;";
	char *q2 = "DELETE FROM player_channel_privileges WHERE player_id = %i;";
	dbi_result res;

	res = dbi_conn_queryf(c->conn, q, r->db_id);
	if (res == NULL)
		logger(LOG_WARN, "db_del_registration : SQL query failed");
	else
		dbi_result_free(res);

	res = dbi_conn_queryf(c->conn, q2, r->db_id);
	if (res == NULL)
		logger(LOG_WARN, "db_del_registration : SQL query failed (2)");
	else
		dbi_result_free(res);
	return 1;
}
