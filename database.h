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

#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "server.h"
#include "configuration.h"
#include "player_channel_privilege.h"

int init_db(struct config *c);
int connect_db(struct config *c);

void db_create_servers(struct config *c, struct array *ss);
int db_create_channels(struct config *c, struct server *s);
int db_create_subchannels(struct config *c, struct server *s);
int db_create_registrations(struct config *c, struct server *s);
int db_create_sv_privileges(struct config *c, struct server *s);
int db_add_registration(struct config *c, struct server *s, struct registration *r);
int db_del_registration(struct config *c, struct server *s, struct registration *r);

int db_unregister_channel(struct config *c, struct channel *ch);
int db_register_channel(struct config *c, struct channel *ch);
int db_update_channel(struct config *c, struct channel *ch);
void db_update_pl_chan_priv(struct config *c, struct player_channel_privilege *tmp_priv);
void db_create_pl_ch_privileges(struct config *c, struct server *s);
void db_del_pl_chan_priv(struct config *c, struct player_channel_privilege *priv);
void db_add_pl_chan_priv(struct config *c, struct player_channel_privilege *priv);

#endif
