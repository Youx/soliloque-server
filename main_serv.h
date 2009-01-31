#ifndef __MAIN_SERV_H__
#define __MAIN_SERV_H__

#include "server.h"
#include <sys/socket.h>

void handle_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s);

#endif
