#include "database.h"

#include <stdio.h>
#include <dbi/dbi.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "configuration.h"
#include "registration.h"

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
		dbi_conn_set_option(c->conn, "dbname", c->db.path);
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
	printf("Connecting to db\n");
	if (dbi_conn_connect(c->conn) < 0) {
		printf("Could not connect. Please check the option settings\n");
		return 0;
	}
	return 1;
}

/**
 * Create servers from a database
 *
 * @param c the config containing the connection info
 *
 * @return the servers followed by a NULL pointer
 */
struct server **db_create_servers(struct config *c)
{
	dbi_result res;
	char *q = "SELECT * FROM servers WHERE active = 1;";
	int nb_serv;
	struct server **ss;
	int i;

	if (connect_db(c) == 0)
		return NULL;

	res = dbi_conn_query(c->conn, q);
	nb_serv = dbi_result_get_numrows(res);
	if (nb_serv == 0) {
		return NULL;
	}
	ss = (struct server **)calloc(nb_serv + 1, sizeof(struct server *));
	if (res) {
		i = 0;
		for (i = 0 ; dbi_result_next_row(res) ; i++) {
			ss[i] = new_server();
			ss[i]->conf = c;

			ss[i]->id = dbi_result_get_uint(res, "id");
			strcpy(ss[i]->password, dbi_result_get_string(res, "password"));
			strcpy(ss[i]->server_name, dbi_result_get_string(res, "name"));
			strcpy(ss[i]->welcome_msg, dbi_result_get_string(res, "welcome_msg"));
			ss[i]->port = dbi_result_get_uint(res, "port");
			/* codecs is a bitfield */
			ss[i]->codecs =
				(dbi_result_get_uint(res, "codec_celp51") << CODEC_CELP_5_1) |
				(dbi_result_get_uint(res, "codec_celp63") << CODEC_CELP_6_3) |
				(dbi_result_get_uint(res, "codec_gsm148") << CODEC_GSM_14_8) |
				(dbi_result_get_uint(res, "codec_gsm164") << CODEC_GSM_16_4) |
				(dbi_result_get_uint(res, "codec_celp52") << CODEC_CELPWin_5_2) |
				(dbi_result_get_uint(res, "codec_speex2150") << CODEC_SPEEX_3_4) |
				(dbi_result_get_uint(res, "codec_speex3950") << CODEC_SPEEX_5_2) |
				(dbi_result_get_uint(res, "codec_speex5950") << CODEC_SPEEX_7_2) |
				(dbi_result_get_uint(res, "codec_speex8000") << CODEC_SPEEX_9_3) |
				(dbi_result_get_uint(res, "codec_speex11000") << CODEC_SPEEX_12_3) |
				(dbi_result_get_uint(res, "codec_speex15000") << CODEC_SPEEX_16_3) |
				(dbi_result_get_uint(res, "codec_speex18200") << CODEC_SPEEX_19_6) |
				(dbi_result_get_uint(res, "codec_speex24600") << CODEC_SPEEX_25_9);
		}
		dbi_result_free(res);
	}
	return ss;
}

/**
 * Make a channel persistent by inserting it into the database
 *
 * @param c the db config
 * @param ch the channel to register
 *
 * @return 0 on failure, 1 on success
 */
int db_register_channel(struct config *c, struct channel *ch)
{
	char *q = "INSERT INTO channels \
		   (server_id, name, topic, description, codec, maxusers, ordr, flag_default, flag_hierarchical, flag_moderated, parent_id, password, created_at) \
		   VALUES (%i,%s,    %s,    %s,          %i,    %i,       %i,   %i,           %i,                %i,             %i,        %s,       NOW());";
	char *name_clean, *topic_clean, *desc_clean, *pass_clean;
	int flag_default, flag_hierar, flag_mod;
	int insert_id;
	dbi_result res;

	printf("Trying to register a channel in the DB\n");
	if (ch->db_id != 0) /* already exists in the db */
		return 0;
	if (connect_db(c) == 0)
		return 0;

	printf("Trying to register a channel in the DB - 1\n");
	/* Secure the input before inserting */
	dbi_conn_quote_string_copy(c->conn, ch->name, &name_clean);
	dbi_conn_quote_string_copy(c->conn, ch->topic, &topic_clean);
	dbi_conn_quote_string_copy(c->conn, ch->desc, &desc_clean);
	dbi_conn_quote_string_copy(c->conn, ch->password, &pass_clean);

	/* better here than in the query function */
	flag_default = (ch->flags & CHANNEL_FLAG_DEFAULT);
	flag_hierar = (ch->flags & CHANNEL_FLAG_SUBCHANNELS);
	flag_mod = (ch->flags & CHANNEL_FLAG_MODERATED);

	if (dbi_conn_queryf(c->conn, q,
			ch->in_server->id, name_clean, topic_clean, desc_clean,
			ch->codec, ch->max_users, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			0xFFFFFFFF, pass_clean) == NULL) {
		printf("Insertion request failed : \n");
		printf(q, ch->in_server->id, name_clean, topic_clean, desc_clean,
			ch->codec, ch->max_users, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			0xFFFFFFFF, pass_clean);
		printf("\n");
	}

	insert_id = dbi_conn_sequence_last(c->conn, NULL);
	ch->db_id = insert_id;

	printf("Trying to register a channel in the DB - 1\n");
	/* free allocated data */
	free(name_clean); free(topic_clean); free(desc_clean); free(pass_clean);

	return 1;
}

/**
 * Unregister by removing it from the database
 *
 * @param c the db config
 * @param ch the channel to unregister
 *
 * @return 1 on success, 0 on failure
 */
int db_unregister_channel(struct config *c, struct channel *ch)
{
	char *q = "DELETE FROM channels WHERE id = %i;";

	if (connect_db(c) == 0)
		return 0;
	dbi_conn_queryf(c->conn, q, ch->db_id);
	
	return 1;
}

/**
 * Go through the database, read and add to the server all the channels
 * stored.
 *
 * @param c the configuration file containing the database connection
 * @param s the server
 */
int db_create_channels(struct config *c, struct server *s)
{
	char *q = "SELECT * FROM channels WHERE server_id = %i;";
	struct channel *ch;
	dbi_result res;
	char *name, *topic, *desc;
	int flags;

	if (connect_db(c) == 0)
		return 0;

	res = dbi_conn_queryf(c->conn, q, s->id);

	if (dbi_result_get_numrows(res) == 0) {
		ch = new_channel("Default", "", "", CHANNEL_FLAG_DEFAULT | CHANNEL_FLAG_UNREGISTERED,
				CODEC_SPEEX_12_3, 0, 128);
		add_channel(s, ch);
	}
	if (res) {
		while (dbi_result_next_row(res)) {
			/* temporary variables to be readable */
			name = dbi_result_get_string_copy(res, "name");
			topic = dbi_result_get_string_copy(res, "topic");
			desc = dbi_result_get_string_copy(res, "description");
			flags = (CHANNEL_FLAG_REGISTERED |
					dbi_result_get_uint(res, "flag_moderated") << CHANNEL_FLAG_MODERATED |
					dbi_result_get_uint(res, "flag_hierarchical") << CHANNEL_FLAG_SUBCHANNELS |
					dbi_result_get_uint(res, "flag_default") << CHANNEL_FLAG_DEFAULT),
			/* create the channel */
			ch = new_channel(name, topic, desc, flags,
					dbi_result_get_uint(res, "codec"),
					dbi_result_get_int(res, "ordr"),
					dbi_result_get_uint(res, "maxusers"));
			ch->db_id = dbi_result_get_uint(res, "id");
			add_channel(s, ch);
			/* free temporary variables */
			free(name); free(topic); free(desc);
		}
		dbi_result_free(res);
	}
	return 1;
}

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

	if (connect_db(c) == 0)
		return 0;

	res = dbi_conn_queryf(c->conn, q, s->id);

	if (res) {
		while (dbi_result_next_row(res)) {
			r = new_registration();
			r->global_flags = dbi_result_get_uint(res, "serveradmin");
			name = dbi_result_get_string_copy(res, "name");
			strncpy(r->name, name, MIN(29, strlen(name)));
			pass = dbi_result_get_string_copy(res, "password");
			strncpy(r->password, pass, MIN(29, strlen(pass)));
			add_registration(s, r);
			/* free temporary variables */
			free(pass); free(name);
		}
		dbi_result_free(res);
	}
	return 1;
}
