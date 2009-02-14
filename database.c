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
		   (server_id, name, topic, description, \
		    codec, maxusers, ordr, \
		    flag_default, flag_hierarchical, flag_moderated, \
		    parent_id, password, created_at) \
		   VALUES \
		   (%i, %s, %s, %s, \
		    %i, %i, %i, \
		    %i, %i, %i, \
		    %i, %s, NOW());";
	char *name_clean, *topic_clean, *desc_clean, *pass_clean;
	int flag_default, flag_hierar, flag_mod;
	int insert_id;
	dbi_result res;

	if (ch->db_id != 0) /* already exists in the db */
		return 0;
	if (connect_db(c) == 0)
		return 0;

	/* Secure the input before inserting */
	dbi_conn_quote_string_copy(c->conn, ch->name, &name_clean);
	dbi_conn_quote_string_copy(c->conn, ch->topic, &topic_clean);
	dbi_conn_quote_string_copy(c->conn, ch->desc, &desc_clean);
	dbi_conn_quote_string_copy(c->conn, ch->password, &pass_clean);

	/* better here than in the query function */
	flag_default = (ch->flags & CHANNEL_FLAG_DEFAULT);
	flag_hierar = (ch->flags & CHANNEL_FLAG_SUBCHANNELS);
	flag_mod = (ch->flags & CHANNEL_FLAG_MODERATED);

	res = dbi_conn_queryf(c->conn, q,
			ch->in_server->id, name_clean, topic_clean, desc_clean,
			ch->codec, ch->players->max_slots, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			0xFFFFFFFF, pass_clean);
	if (res == NULL) {
		printf("Insertion request failed : \n");
		printf(q, ch->in_server->id, name_clean, topic_clean, desc_clean,
			ch->codec, ch->players->max_slots, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			0xFFFFFFFF, pass_clean);
		printf("\n");
	}

	insert_id = dbi_conn_sequence_last(c->conn, NULL);
	ch->db_id = insert_id;

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

/**
 * Go through the database, read and add to the server all
 * the server permissions stored.
 *
 * @param c the configuration of the db
 * @param s the server
 */
int db_create_sv_privileges(struct config *c, struct server *s)
{
	char *q = "SELECT * FROM server_privileges WHERE server_id = %i;";
	dbi_result res;
	const char *group;
	int g;
	struct server_privileges *sp = s->privileges;

	printf("Loading server privileges.\n");
	if (connect_db(c) == 0)
		return 0;

	res = dbi_conn_queryf(c->conn, q, s->id);
	if (res) {
		while (dbi_result_next_row(res)) {
			printf("sp row...\n");
			/* Get the id of the group from the string */
			group = dbi_result_get_string(res, "user_group");
			if (strcmp(group, "server_admin") == 0) {
				g = PRIV_SERVER_ADMIN;
			} else if (strcmp(group, "channel_admin") == 0) {
				g = PRIV_CHANNEL_ADMIN;
			} else if (strcmp(group, "operator") == 0) {
				g = PRIV_OPERATOR;
			} else if (strcmp(group, "voice") == 0) {
				g = PRIV_VOICE;
			} else if (strcmp(group, "registered") == 0) {
				g = PRIV_REGISTERED;
			} else if (strcmp(group, "anonymous") == 0) {
				g = PRIV_ANONYMOUS;
			} else {
				printf("(EE) server_privileges.user_group = %s, \
						expected : server_admin, channel_admin, \
						operator, voice, registered, anonymous.\n",
						group);
				continue;
			}
			printf("GROUP : %i\n", g);
			/* Copy all privileges to the server... */
			sp->priv[g][SP_ADM_DEL_SERVER] = dbi_result_get_uint(res, "adm_del_server");
			sp->priv[g][SP_ADM_ADD_SERVER] = dbi_result_get_uint(res, "adm_add_server");
			sp->priv[g][SP_ADM_LIST_SERVERS] = dbi_result_get_uint(res, "adm_list_servers");
			sp->priv[g][SP_ADM_SET_PERMISSIONS] = dbi_result_get_uint(res, "adm_set_permissions");
			sp->priv[g][SP_ADM_CHANGE_USER_PASS] = dbi_result_get_uint(res, "adm_change_user_pass");
			sp->priv[g][SP_ADM_CHANGE_OWN_PASS] = dbi_result_get_uint(res, "adm_change_own_pass");
			sp->priv[g][SP_ADM_LIST_REGISTRATIONS] = dbi_result_get_uint(res, "adm_list_registrations");
			sp->priv[g][SP_ADM_REGISTER_PLAYER] = dbi_result_get_uint(res, "adm_register_player");

			sp->priv[g][SP_ADM_CHANGE_SERVER_CODECS] = dbi_result_get_uint(res, "adm_change_server_codecs");
			sp->priv[g][SP_ADM_CHANGE_SERVER_TYPE] = dbi_result_get_uint(res, "adm_change_server_type");
			sp->priv[g][SP_ADM_CHANGE_SERVER_PASS] = dbi_result_get_uint(res, "adm_change_server_pass");
			sp->priv[g][SP_ADM_CHANGE_SERVER_WELCOME] = dbi_result_get_uint(res, "adm_change_server_welcome");
			sp->priv[g][SP_ADM_CHANGE_SERVER_MAXUSERS] = dbi_result_get_uint(res, "adm_change_server_maxusers");
			sp->priv[g][SP_ADM_CHANGE_SERVER_NAME] = dbi_result_get_uint(res, "adm_change_server_name");
			sp->priv[g][SP_ADM_CHANGE_WEBPOST_URL] = dbi_result_get_uint(res, "adm_change_webpost_url");
			sp->priv[g][SP_ADM_CHANGE_SERVER_PORT] = dbi_result_get_uint(res, "adm_change_server_port");

			sp->priv[g][SP_ADM_START_SERVER] = dbi_result_get_uint(res, "adm_start_server");
			sp->priv[g][SP_ADM_STOP_SERVER] = dbi_result_get_uint(res, "adm_stop_server");
			sp->priv[g][SP_ADM_MOVE_PLAYER] = dbi_result_get_uint(res, "adm_move_player");
			sp->priv[g][SP_ADM_BAN_IP] = dbi_result_get_uint(res, "adm_ban_ip");

			sp->priv[g][SP_CHA_DELETE] = dbi_result_get_uint(res, "cha_delete");
			sp->priv[g][SP_CHA_CREATE_MODERATED] = dbi_result_get_uint(res, "cha_create_moderated");
			sp->priv[g][SP_CHA_CREATE_SUBCHANNELED] = dbi_result_get_uint(res, "cha_create_subchanneled");
			sp->priv[g][SP_CHA_CREATE_DEFAULT] = dbi_result_get_uint(res, "cha_create_default");
			sp->priv[g][SP_CHA_CREATE_UNREGISTERED] = dbi_result_get_uint(res, "cha_create_unregistered");
			sp->priv[g][SP_CHA_CREATE_REGISTERED] = dbi_result_get_uint(res, "cha_create_registered");
			sp->priv[g][SP_CHA_JOIN_REGISTERED] = dbi_result_get_uint(res, "cha_join_registered");

			sp->priv[g][SP_CHA_JOIN_WO_PASS] = dbi_result_get_uint(res, "cha_join_wo_pass");
			sp->priv[g][SP_CHA_CHANGE_CODEC] = dbi_result_get_uint(res, "cha_change_codec");
			sp->priv[g][SP_CHA_CHANGE_MAXUSERS] = dbi_result_get_uint(res, "cha_change_maxusers");
			sp->priv[g][SP_CHA_CHANGE_ORDER] = dbi_result_get_uint(res, "cha_change_order");
			sp->priv[g][SP_CHA_CHANGE_DESC] = dbi_result_get_uint(res, "cha_change_desc");
			sp->priv[g][SP_CHA_CHANGE_TOPIC] = dbi_result_get_uint(res, "cha_change_topic");
			sp->priv[g][SP_CHA_CHANGE_PASS] = dbi_result_get_uint(res, "cha_change_pass");
			sp->priv[g][SP_CHA_CHANGE_NAME] = dbi_result_get_uint(res, "cha_change_name");

			sp->priv[g][SP_PL_GRANT_ALLOWREG] = dbi_result_get_uint(res, "pl_grant_allowreg");
			sp->priv[g][SP_PL_GRANT_VOICE] = dbi_result_get_uint(res, "pl_grant_voice");
			sp->priv[g][SP_PL_GRANT_AUTOVOICE] = dbi_result_get_uint(res, "pl_grant_autovoice");
			sp->priv[g][SP_PL_GRANT_OP] = dbi_result_get_uint(res, "pl_grant_op");
			sp->priv[g][SP_PL_GRANT_AUTOOP] = dbi_result_get_uint(res, "pl_grant_autoop");
			sp->priv[g][SP_PL_GRANT_CA] = dbi_result_get_uint(res, "pl_grant_ca");
			sp->priv[g][SP_PL_GRANT_SA] = dbi_result_get_uint(res, "pl_grant_sa");

			sp->priv[g][SP_PL_REGISTER_PLAYER] = dbi_result_get_uint(res, "pl_register_player");
			sp->priv[g][SP_PL_REVOKE_ALLOWREG] = dbi_result_get_uint(res, "pl_revoke_allowreg");
			sp->priv[g][SP_PL_REVOKE_VOICE] = dbi_result_get_uint(res, "pl_revoke_voice");
			sp->priv[g][SP_PL_REVOKE_AUTOVOICE] = dbi_result_get_uint(res, "pl_revoke_autovoice");
			sp->priv[g][SP_PL_REVOKE_OP] = dbi_result_get_uint(res, "pl_revoke_op");
			sp->priv[g][SP_PL_REVOKE_AUTOOP] = dbi_result_get_uint(res, "pl_revoke_autoop");
			sp->priv[g][SP_PL_REVOKE_CA] = dbi_result_get_uint(res, "pl_revoke_ca");
			sp->priv[g][SP_PL_REVOKE_SA] = dbi_result_get_uint(res, "pl_revoke_sa");

			sp->priv[g][SP_PL_ALLOW_SELF_REG] = dbi_result_get_uint(res, "pl_allow_self_reg");
			sp->priv[g][SP_PL_DEL_REGISTRATION] = dbi_result_get_uint(res, "pl_del_registration");

			sp->priv[g][SP_OTHER_CH_COMMANDER] = dbi_result_get_uint(res, "other_ch_commander");
			sp->priv[g][SP_OTHER_CH_KICK] = dbi_result_get_uint(res, "other_ch_kick");
			sp->priv[g][SP_OTHER_SV_KICK] = dbi_result_get_uint(res, "other_sv_kick");
			sp->priv[g][SP_OTHER_TEXT_PL] = dbi_result_get_uint(res, "other_text_pl");
			sp->priv[g][SP_OTHER_TEXT_ALL_CH] = dbi_result_get_uint(res, "other_text_all_ch");
			sp->priv[g][SP_OTHER_TEXT_IN_CH] = dbi_result_get_uint(res, "other_text_in_ch");
			sp->priv[g][SP_OTHER_TEXT_ALL] = dbi_result_get_uint(res, "other_text_all");
		}
		dbi_result_free(res);
	}
	return 1;
}
