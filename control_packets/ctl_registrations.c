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

#include "control_packet.h"
#include "log.h"
#include "packet_tools.h"
#include "server_stat.h"
#include "acknowledge_packet.h"
#include "registration.h"
#include "database.h"

#include <errno.h>
#include <string.h>
#include <openssl/sha.h>

/**
 * Handle a packet to create a new registration with a name, password
 * and server admin flag.
 *
 * @param data the packet
 * @param len the length of the packet
 * @param pl the player who issued the request
 */
void *c_req_create_registration(char *data, unsigned int len, struct player *pl)
{
	char *name, *pass;
	char server_admin;
	struct registration *reg;
	struct server *s;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	char *digest_readable, *ptr;

	s = pl->in_chan->in_server;

	send_acknowledge(pl);
	if (player_has_privilege(pl, SP_PL_REGISTER_PLAYER, NULL)) {
		ptr = data + 24;
		name = rstaticstring(29, &ptr);
		pass = rstaticstring(29, &ptr);
		server_admin = ru8(&ptr);
		if (name == NULL || pass == NULL) {
			ERROR("c_req_create_registration, strndup failed : %s.", strerror(errno));
			if (name != NULL)
				free(name);
			if (pass != NULL)
				free(pass);
			return NULL;
		}
		reg = new_registration();
		strcpy(reg->name, name);
		/* hash the password */
		SHA256((unsigned char *)pass, strlen(pass), digest);
		digest_readable = ustrtohex(digest, SHA256_DIGEST_LENGTH);
		strcpy(reg->password, digest_readable);
		free(digest_readable);

		reg->global_flags = server_admin;
		add_registration(s, reg);
		/* database callback to insert a new registration */
		db_add_registration(s->conf, s, reg);

		free(name);
		free(pass);
	}

	return NULL;
}

/**
 * Handle a packet to create a new registration with a name, password
 * and server admin flag and associate it to the player.
 *
 * @param data the packet
 * @param len the length of the packet
 * @param pl the player who issued the request
 */
void *c_req_register_player(char *data, unsigned int len, struct player *pl)
{
	char *name, *pass;
	struct registration *reg;
	struct server *s;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	char *digest_readable, *ptr;
	struct channel *ch;
	struct player_channel_privilege *priv;
	size_t iter, iter2;

	s = pl->in_chan->in_server;

	INFO("c_req_register_player : registering player");
	send_acknowledge(pl);
	if (player_has_privilege(pl, SP_PL_ALLOW_SELF_REG, NULL)
			|| (pl->global_flags & GLOBAL_FLAG_ALLOWREG)) {
		INFO("c_req_register_player : privileges OK");
		ptr = data + 24;
		name = rstaticstring(29, &ptr);
		pass = rstaticstring(29, &ptr);

		if (name == NULL || pass == NULL) {
			ERROR("c_req_register_player, strndup failed : %s.", strerror(errno));
			if (name != NULL)
				free(name);
			if (pass != NULL)
				free(pass);
			return NULL;
		}
		reg = new_registration();
		strcpy(reg->name, name);
		/* hash the password */
		SHA256((unsigned char *)pass, strlen(pass), digest);
		digest_readable = ustrtohex(digest, SHA256_DIGEST_LENGTH);
		strcpy(reg->password, digest_readable);
		free(digest_readable);

		add_registration(s, reg);
		/* associate the player with this new registration */
		pl->reg = reg;
		ar_each(struct channel *, ch, iter, s->chans)
			if (!(ch->flags & CHANNEL_FLAG_UNREGISTERED)) {
				ar_each(struct player_channel_privilege *, priv, iter2, ch->pl_privileges)
					if (priv->reg == PL_CH_PRIV_UNREGISTERED && priv->pl_or_reg.pl == pl) {
						priv->reg = PL_CH_PRIV_REGISTERED;
						priv->pl_or_reg.reg = reg;
					}
				ar_end_each;
			}
		ar_end_each;
		/* database callback to insert a new registration */
		db_add_registration(s->conf, s, reg);
		s_notify_player_sv_right_changed(NULL, pl, 2, 0);

		free(name);
		free(pass);
	}

	return NULL;
}
