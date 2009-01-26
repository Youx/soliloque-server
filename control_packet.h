#ifndef __CONTROL_PACKET_H__
#define __CONTROL_PACKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <resolv.h>
#include "player.h"
#include "server.h"

void *c_req_chans(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void s_notify_new_player(struct player *pl, struct server *s);
void *c_req_leave(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void *c_req_kick_server(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void *c_req_kick_channel(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void *c_req_switch_channel(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void *c_req_delete_channel(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void *c_req_ban(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void *c_req_list_bans(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);

#endif
