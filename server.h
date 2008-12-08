#ifndef __SERVER_H__
#define __SERVER_H__

#include "channel.h"
#include "array.h"

struct server {
	struct array *chans;
	struct array *players;
	
	char password[30];
};


struct server * new_server();

/* Server - channel functions */
struct channel * get_channel_by_id(struct server * serv, uint32_t id);
int add_channel(struct server * serv, struct channel * chan);
int destroy_channel_by_id(struct server * serv, uint32_t id);
struct channel * get_default_channel(struct server * serv);

/* Server - player functions */
int add_player(struct server * serv, struct player * pl);


void print_server(struct server * s);

#endif
