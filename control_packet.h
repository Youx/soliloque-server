#ifndef __CONTROL_PACKET_H__
#define __CONTROL_PACKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <resolv.h>
#include "player.h"
#include "server.h"

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

#endif
