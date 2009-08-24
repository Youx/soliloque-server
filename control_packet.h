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

#ifndef __CONTROL_PACKET_H__
#define __CONTROL_PACKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include "player.h"
#include "server.h"

#define PKT_TYPE_CTL		0xbef0

#define CTL_LIST_CH		0x0006	/* list channels */
#define CTL_LIST_PL		0x0007	/* list players */
#define CTL_CREATE_PL		0x0064	/* new player */
#define CTL_PLAYERLEFT		0x0065	/* player left/kick/ban */
#define CTL_CHKICK_PL		0x0066	/* kick from channel */
#define CTL_SWITCHCHAN		0x0067	/* player switched channels */
#define CTL_CHANGE_PL_STATUS	0x0068	/* player status changed */
#define CTL_CHANGE_PL_CHPRIV	0x006a	/* player channel priv changed */
#define CTL_CHANGE_PL_SVPRIV	0x006b	/* player servr priv changed */
#define CTL_PLAYER_MOVED	0x006d	/* move a player from a channel to another */
#define CTL_CREATE_CH		0x006e	/* new channel */
#define CTL_CHANGE_CH_NAME	0x006f	/* channel name changed */
#define CTL_CHANGE_CH_TOPIC	0x0070	/* channel topic changed */
#define CTL_CHANGE_CH_FLAGS_CODEC 0x0071 /* change channel flags or codec */
#define CTL_CHANGE_CH_DESC	0x0072	/* channel desc changed */
#define CTL_CHANDELETE		0x0073	/* channel deleted */
#define CTL_CHANGE_CH_MAX_USERS	0x0074	/* channel max users changed */
#define CTL_CHANGE_CH_ORDER	0x0075	/* channel order changed */
#define CTL_MESSAGE		0x0082	/* message (all/channel/private) */
#define CTL_CHANDELETE_ERROR	0xff93	/* error deleting channel */
#define CTL_PLAYERSTATS		0x0194	/* player stats */
#define CTL_SERVSTATS		0x0196	/* server stats */
#define CTL_BANLIST		0x019b	/* server list of bans */

void *c_req_chans(char *data, unsigned int len, struct player *pl);
void s_notify_new_player(struct player *pl);
void *c_req_leave(char *data, unsigned int len, struct player *pl);
void *c_req_kick_server(char *data, unsigned int len, struct player *pl);
void *c_req_kick_channel(char *data, unsigned int len, struct player *pl);
void *c_req_switch_channel(char *data, unsigned int len, struct player *pl);
void *c_req_delete_channel(char *data, unsigned int len, struct player *pl);
void *c_req_ban(char *data, unsigned int len, struct player *pl);
void *c_req_list_bans(char *data, unsigned int len, struct player *pl);
void *c_req_remove_ban(char *data, unsigned int len, struct player *pl);
void *c_req_ip_ban(char *data, unsigned int len, struct player *pl);
void *c_req_server_stats(char *data, unsigned int len, struct player *pl);
void *c_req_change_player_ch_priv(char *data, unsigned int len, struct player *pl);
void *c_req_change_player_sv_right(char *data, unsigned int len, struct player *pl);
void *c_req_change_player_attr(char *data, unsigned int len, struct player *pl);
void *c_req_send_message(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_name(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_topic(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_desc(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_flag_codec(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_pass(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_order(char *data, unsigned int len, struct player *pl);
void *c_req_change_chan_max_users(char *data, unsigned int len, struct player *pl);
void *c_req_create_channel(char *data, unsigned int len, struct player *pl);
void *c_req_player_stats(char *data, unsigned int len, struct player *pl);
void *c_req_create_registration(char *data, unsigned int len, struct player *pl);
void *c_req_register_player(char *data, unsigned int len, struct player *pl);
void *c_req_move_player(char *data, unsigned int len, struct player *pl);

void s_notify_player_sv_right_changed(struct player *pl, struct player *tgt, char right, char on_off);
void s_notify_player_left(struct player *p);


#endif
