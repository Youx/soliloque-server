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
#include "server_privileges.h"
#include "player_channel_privilege.h"
#include "registration.h"

#include <errno.h>
#include <string.h>
#include <dbi/dbi.h>

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
