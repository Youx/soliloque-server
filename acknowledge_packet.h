#ifndef __ACKNOWLEDGE_PACKET_H__
#define __ACKNOWLEDGE_PACKET_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>

#include "player.h"

void send_acknowledge(struct player *pl);

#endif
