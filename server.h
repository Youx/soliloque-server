#ifndef __SERVER_H__
#define __SERVER_H__

#include "channel.h"
#include "player.h"
#include "array.h"

struct server {
	struct array *chans;
	struct array *players;
	
	char password[30];
	char server_name[30];
	char machine[30];
	uint16_t version[4]/* = {2,0,20,1}*/;

	int socket_desc;
	int port;
};


struct server *new_server();

/* Server - channel functions */
struct channel *get_channel_by_id(struct server *serv, uint32_t id);
int add_channel(struct server *serv, struct channel *chan);
int destroy_channel_by_id(struct server *serv, uint32_t id);
struct channel *get_default_channel(struct server *serv);

/* Server - player functions */
struct player *get_player_by_ids(struct server *s, uint32_t pub_id, uint32_t priv_id);
int add_player(struct server *serv, struct player *pl);
void remove_player(struct server *s, struct player *p);

void print_server(struct server *s);

#endif
