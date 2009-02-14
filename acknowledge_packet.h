#ifndef __ACKNOWLEDGE_PACKET_H__
#define __ACKNOWLEDGE_PACKET_H__

#include <sys/socket.h>
#include <sys/types.h>

#include "player.h"

#define PKT_TYPE_ACK 0xbef1

void send_acknowledge(struct player *pl);

#endif
