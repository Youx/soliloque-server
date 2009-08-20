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

#include <stdio.h>
#include <dbi/dbi.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "server.h"
#include "configuration.h"
#include "registration.h"
#include "player_channel_privilege.h"
#include "log.h"

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
	logger(LOG_INFO, "Connecting to db");
	if (dbi_conn_connect(c->conn) < 0) {
		logger(LOG_ERR, "Could not connect. Please check the option settings");
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

	res = dbi_conn_query(c->conn, q);
	nb_serv = dbi_result_get_numrows(res);
	if (nb_serv == 0) {
		return NULL;
	}
	ss = (struct server **)calloc(nb_serv + 1, sizeof(struct server *));
	if (ss == NULL) {
		logger(LOG_WARN, "db_create_server : calloc failed : %s.", strerror(errno));
		return NULL;
	}

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
		    parent_id, password) \
		   VALUES \
		   (%i, %s, %s, %s, \
		    %i, %i, %i, \
		    %i, %i, %i, \
		    %i, %s);";
	char *name_clean, *topic_clean, *desc_clean, *pass_clean;
	int flag_default, flag_hierar, flag_mod;
	int insert_id, parent_id;
	size_t iter;
	struct channel *tmp_ch;
	struct player_channel_privilege *priv;
	dbi_result res;

	if (ch->db_id != 0) /* already exists in the db */
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
	
	/* Add the ID of the parent or -1 */
	if (ch->parent == NULL)
		parent_id = 0xFFFFFFFF;
	else
		parent_id = ch->parent->db_id;

	res = dbi_conn_queryf(c->conn, q,
			ch->in_server->id, name_clean, topic_clean, desc_clean,
			ch->codec, ch->players->max_slots, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			parent_id, pass_clean);
	if (res == NULL) {
		logger(LOG_ERR, "Insertion request failed : ");
		logger(LOG_ERR, q, ch->in_server->id, name_clean, topic_clean, desc_clean,
			ch->codec, ch->players->max_slots, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			parent_id, pass_clean);
	}

	insert_id = dbi_conn_sequence_last(c->conn, NULL);
	ch->db_id = insert_id;

	/* Register all the subchannels */
	if (ch_getflags(ch) & CHANNEL_FLAG_SUBCHANNELS) {
		ar_each(struct channel *, tmp_ch, iter, ch->subchannels)
			db_register_channel(c, tmp_ch);
		ar_end_each;	
	}

	/* add all the player privileges for this channel */
	ar_each(struct player_channel_privilege *, priv, iter, ch->pl_privileges)
		if (priv->reg == PL_CH_PRIV_REGISTERED)
			db_add_pl_chan_priv(c, priv);
	ar_end_each;

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
	size_t iter;
	struct channel *tmp_ch;
	struct player_channel_privilege *priv;

	dbi_conn_queryf(c->conn, q, ch->db_id);
	/* remove all the player privileges for this channel */
	ar_each(struct player_channel_privilege *, priv, iter, ch->pl_privileges)
		if (priv->reg == PL_CH_PRIV_REGISTERED)
			db_del_pl_chan_priv(c, priv);
	ar_end_each;

	/* unregister all the subchannels */
	if (ch_getflags(ch) & CHANNEL_FLAG_SUBCHANNELS) {
		ar_each(struct channel *, tmp_ch, iter, ch->subchannels)
			db_unregister_channel(c, tmp_ch);
		ar_end_each;	
	}
	ch->db_id = 0;

	return 1;
}

/**
 * Update a registered channel's fields.
 *
 * @param c the db config
 * @param ch the channel to update
 *
 * @return 1 on success
 */
int db_update_channel(struct config *c, struct channel *ch)
{
	char *q = "UPDATE channels SET name = %s, topic = %s, description = %s, \
		    codec = %i, maxusers = %i, ordr = %i, \
		    flag_default = %i, flag_hierarchical = %i, flag_moderated = %i, \
		    password = %s \
		    WHERE id = %i;";
	char *name_clean, *topic_clean, *desc_clean, *pass_clean;
	int flag_default, flag_hierar, flag_mod;
	dbi_result res;

	if (ch->db_id == 0) /* does not exist in the db */
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
			name_clean, topic_clean, desc_clean,
			ch->codec, ch->players->max_slots, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			pass_clean, ch->db_id);
	if (res == NULL) {
		logger(LOG_ERR, "Insertion request failed : ");
		logger(LOG_ERR, q, name_clean, topic_clean, desc_clean,
			ch->codec, ch->players->max_slots, ch->sort_order,
			flag_default, flag_hierar, flag_mod,
			pass_clean, ch->db_id);
	}
	free(name_clean); free(topic_clean); free(desc_clean); free(pass_clean);

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
	char *q = "SELECT * FROM channels WHERE server_id = %i AND parent_id = -1;";
	struct channel *ch;
	dbi_result res;
	char *name, *topic, *desc;
	int flags;

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
			logger(LOG_INFO, "flag_hierarchical = %i", dbi_result_get_uint(res, "flag_hierarchical"));
			flags = (0 & ~CHANNEL_FLAG_UNREGISTERED);
			if (dbi_result_get_uint(res, "flag_moderated"))
				flags |= CHANNEL_FLAG_MODERATED;
			if (dbi_result_get_uint(res, "flag_hierarchical"))
				flags |= CHANNEL_FLAG_SUBCHANNELS;
			if (dbi_result_get_uint(res, "flag_default"))
				flags |= CHANNEL_FLAG_DEFAULT;
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

int db_create_subchannels(struct config *c, struct server *s)
{
	char *q = "SELECT * FROM channels WHERE server_id = %i AND parent_id != -1;";
	struct channel *ch, *parent;
	dbi_result res;
	char *name, *topic, *desc;
	int parent_db_id;

	res = dbi_conn_queryf(c->conn, q, s->id);

	if (res) {
		while (dbi_result_next_row(res)) {
			/* temporary variables to be readable */
			name = dbi_result_get_string_copy(res, "name");
			topic = dbi_result_get_string_copy(res, "topic");
			desc = dbi_result_get_string_copy(res, "description");
			/* create the sub channel */
			ch = new_channel(name, topic, desc, 0x00,
					dbi_result_get_uint(res, "codec"),
					dbi_result_get_int(res, "ordr"),
					dbi_result_get_uint(res, "maxusers"));
			ch->db_id = dbi_result_get_uint(res, "id");

			parent_db_id = dbi_result_get_uint(res, "parent_id");

			parent = get_channel_by_db_id(s, parent_db_id);
			if (parent == NULL) {
				logger(LOG_WARN, "db_create_subchannels, channel with db_id %i does not exist.",
						parent_db_id);
				destroy_channel(ch);
			} else if (parent->parent != NULL) {
				logger(LOG_WARN, "db_create_subchannels, a subchannel can not have subchannels.");
				destroy_channel(ch);
			} else if ((parent->flags & CHANNEL_FLAG_SUBCHANNELS) == 0) {
				logger(LOG_WARN, "db_create_subchannels, channel %s can not have subchannel.",
						parent->name);
				destroy_channel(ch);
			} else {
				add_channel(s, ch);
				channel_add_subchannel(parent, ch);
			}
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

	dbi_conn_quote_string_copy(c->conn, r->name, &quoted_name);
	dbi_conn_quote_string_copy(c->conn, r->password, &quoted_pass);

	res = dbi_conn_queryf(c->conn, req, s->id, r->global_flags, quoted_name, quoted_pass);
	dbi_result_free(res);
	free(quoted_pass);
	free(quoted_name);

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

	logger(LOG_INFO, "Loading server privileges.");

	res = dbi_conn_queryf(c->conn, q, s->id);
	if (res) {
		while (dbi_result_next_row(res)) {
			logger(LOG_INFO, "sp row...");
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
				logger(LOG_ERR, "server_privileges.user_group = %s, \
						expected : server_admin, channel_admin, \
						operator, voice, registered, anonymous.",
						group);
				continue;
			}
			logger(LOG_INFO, "GROUP : %i", g);
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

void db_create_pl_ch_privileges(struct config *c, struct server *s)
{
	dbi_result res;
	int flags;
	size_t iter, iter2;
	int reg_id;
	struct channel *ch;
	struct registration *reg;
	struct player_channel_privilege *tmp_priv;
	char *q = "SELECT * FROM player_channel_privileges WHERE channel_id = %i;";


	logger(LOG_INFO, "Reading player channel privileges...");
	ar_each(struct channel *, ch, iter, s->chans)
		if (!(ch->flags & CHANNEL_FLAG_UNREGISTERED)) {
			res = dbi_conn_queryf(c->conn, q, ch->db_id);
			if (res) {
				while (dbi_result_next_row(res)) {
					tmp_priv = new_player_channel_privilege();
					tmp_priv->ch = ch;
					flags = 0;
					if (dbi_result_get_uint(res, "channel_admin"))
						flags |= CHANNEL_PRIV_CHANADMIN;
					if (dbi_result_get_uint(res, "operator"))
						flags |= CHANNEL_PRIV_OP;
					if (dbi_result_get_uint(res, "voice"))
						flags |= CHANNEL_PRIV_VOICE;
					if (dbi_result_get_uint(res, "auto_operator"))
						flags |= CHANNEL_PRIV_AUTOOP;
					if (dbi_result_get_uint(res, "auto_voice"))
						flags |= CHANNEL_PRIV_AUTOVOICE;
					tmp_priv->flags = flags;
					tmp_priv->reg = PL_CH_PRIV_REGISTERED;
					reg_id = dbi_result_get_uint(res, "player_id");
					ar_each(struct registration *, reg, iter2, s->regs)
						if (reg->db_id == reg_id)
							tmp_priv->pl_or_reg.reg = reg;
					ar_end_each;
					if (tmp_priv->pl_or_reg.reg != NULL)
						add_player_channel_privilege(ch, tmp_priv);
					else
						free(tmp_priv);
				}
				dbi_result_free(res);
			} else {
				logger(LOG_WARN, "db_create_pl_ch_privileges : SQL query failed.");
			}
		}
	ar_end_each;
}

void db_update_pl_chan_priv(struct config *c, struct player_channel_privilege *tmp_priv)
{
	dbi_result res;
	char *q = "UPDATE player_channel_privileges \
			SET channel_admin = %i, operator = %i, voice = %i, auto_operator = %i, auto_voice = %i \
			WHERE player_id = %i AND channel_id = %i;" ;

	logger(LOG_INFO, "db_update_pl_chan_priv");
	if (tmp_priv->reg != PL_CH_PRIV_REGISTERED) {
		logger(LOG_WARN, "db_update_pl_chan_priv : trying to update a pl_ch_priv that is marked as unregistered. This should not happen!");
		return;
	}
	if (tmp_priv->pl_or_reg.reg == NULL) {
		logger(LOG_WARN, "db_update_pl_chan_priv : registration is NULL. This should not happen!");
		return;
	}
	if (tmp_priv->ch == NULL) {
		logger(LOG_WARN, "db_update_pl_chan_priv : channel is NULL. This should not happen!");
		return;
	}
	res = dbi_conn_queryf(c->conn, q,
			tmp_priv->flags & CHANNEL_PRIV_CHANADMIN,
			tmp_priv->flags & CHANNEL_PRIV_OP,
			tmp_priv->flags & CHANNEL_PRIV_VOICE,
			tmp_priv->flags & CHANNEL_PRIV_AUTOOP,
			tmp_priv->flags & CHANNEL_PRIV_AUTOVOICE,
			tmp_priv->pl_or_reg.reg->db_id,
			tmp_priv->ch->db_id);
			
	if (res == NULL) {
		logger(LOG_WARN, "db_update_pl_chan_priv : SQL query failed.");
		logger(LOG_WARN, q, tmp_priv->flags & CHANNEL_PRIV_CHANADMIN, tmp_priv->flags & CHANNEL_PRIV_OP,
			tmp_priv->flags & CHANNEL_PRIV_VOICE, tmp_priv->flags & CHANNEL_PRIV_AUTOOP,
			tmp_priv->flags & CHANNEL_PRIV_AUTOVOICE,
			tmp_priv->pl_or_reg.reg->db_id, tmp_priv->ch->db_id);
	} else {
		dbi_result_free(res);
	}
}

void db_add_pl_chan_priv(struct config *c, struct player_channel_privilege *priv)
{
	dbi_result res;
	char *q = "INSERT INTO player_channel_privileges \
		   (player_id, channel_id, channel_admin, operator, voice, auto_operator, auto_voice) \
		   VALUES (%i, %i, %i, %i, %i, %i, %i)";

	if (priv->reg != PL_CH_PRIV_REGISTERED) {
		logger(LOG_WARN, "db_add_pl_chan_priv : trying to add a pl_ch_priv that is marked as unregistered. This should not happen!");
		return;
	}
	if (priv->pl_or_reg.reg == NULL) {
		logger(LOG_WARN, "db_add_pl_chan_priv : registration is NULL. This should not happen!");
		return;
	}
	if (priv->ch == NULL) {
		logger(LOG_WARN, "db_add_pl_chan_priv : channel is NULL. This should not happen!");
		return;
	}
	logger(LOG_INFO, "registering a new player channel privilege.");

	res = dbi_conn_queryf(c->conn, q, priv->pl_or_reg.reg->db_id, priv->ch->db_id,
			priv->flags & CHANNEL_PRIV_CHANADMIN, priv->flags & CHANNEL_PRIV_OP, priv->flags & CHANNEL_PRIV_VOICE,
			priv->flags & CHANNEL_PRIV_AUTOOP, priv->flags & CHANNEL_PRIV_AUTOVOICE);
	if (res == NULL)
		logger(LOG_WARN, "db_add_pl_chan_priv : SQL query failed.");
	else
		dbi_result_free(res);
}

void db_del_pl_chan_priv(struct config *c, struct player_channel_privilege *priv)
{
	dbi_result res;
	char *q = "DELETE FROM player_channel_privileges \
			WHERE player_id = %i AND channel_id = %i;";

	if (priv->reg != PL_CH_PRIV_REGISTERED) {
		logger(LOG_WARN, "db_del_pl_chan_priv : trying to delete a pl_ch_priv that is marked as unregistered. This should not happen!");
		return;
	}
	if (priv->pl_or_reg.reg == NULL) {
		logger(LOG_WARN, "db_del_pl_chan_priv : registration is NULL. This should not happen!");
		return;
	}
	if (priv->ch == NULL) {
		logger(LOG_WARN, "db_del_pl_chan_priv : channel is NULL. This should not happen!");
		return;
	}
	logger(LOG_INFO, "unregistering a player channel privilege");

	res = dbi_conn_queryf(c->conn, q, priv->pl_or_reg.reg->db_id, priv->ch->db_id);
	if (res == NULL)
		logger(LOG_WARN, "db_del_pl_chan_priv : SQL query failed.");
	else
		dbi_result_free(res);
}
