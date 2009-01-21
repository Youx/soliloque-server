#ifndef __CONTROL_PACKET_H__
#define __CONTROL_PACKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <resolv.h>
#include "player.h"
#include "server.h"

void *c_req_chans(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
void s_notify_new_player(struct player *pl, struct server *s);

#endif
