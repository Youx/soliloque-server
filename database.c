#include "database.h"

#include <stdio.h>
#include <dbi/dbi.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "configuration.h"

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
		/*printf("host -> %s\n", c->db.connection.host);
		printf("username -> %s\n", c->db.connection.user);
		printf("password -> %s\n", c->db.connection.pass);
		printf("dbname -> %s\n", c->db.connection.db);
		printf("port -> %i\n", c->db.connection.port);*/
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

int db_update_channel(struct config *c, struct channel *ch)
{


}

int db_unregister_channel(struct config *c, struct channel *ch)
{

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

	printf("C\n");
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
			add_channel(s, ch);
			/* free temporary variables */
			free(name); free(topic); free(desc);
		}
	}
	return 1;
}

