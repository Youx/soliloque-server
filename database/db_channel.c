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

#include <dbi/dbi.h>

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
	char *q2 = "DELETE FROM player_channel_privileges WHERE channel_id = %i;";
	size_t iter;
	struct channel *tmp_ch;

	dbi_conn_queryf(c->conn, q, ch->db_id);
	/* remove all the player privileges for this channel */
	dbi_conn_queryf(c->conn, q2, ch->db_id);

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
