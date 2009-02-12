#ifndef __SERVER_H__
#define __SERVER_H__

#include "ban.h"
#include "channel.h"
#include "player.h"
#include "array.h"
#include "server_privileges.h"

#include <pthread.h>
#include <poll.h>

#define ERROR_IF(cond) \
	if(cond) { \
		printf("(EE) %s", strerror(errno)); \
		exit(1); \
	}

#define WARNING_IF(cond) \
	if(cond) { \
		printf("(WW) %s", strerror(errno)); \
	}

struct server {
	uint32_t id;

	struct array *chans;
	struct array *players;
	struct array *bans;
	struct array *regs;
	struct server_stat *stats;

	char password[30];
	char server_name[30];
	char machine[30];
	char welcome_msg[256];
	uint16_t version[4]/* = {2,0,20,1}*/;

	int socket_desc;
	int port;
	int codecs;

	struct server_privileges *privileges;

        struct pollfd socket_poll;
	pthread_t main_thread;
	struct config *conf;
};


struct server *new_server();

/* Server - channel functions */
struct channel *get_channel_by_id(struct server *serv, uint32_t id);
int add_channel(struct server *serv, struct channel *chan);
int destroy_channel_by_id(struct server *serv, uint32_t id);
struct channel *get_default_channel(struct server *serv);

/* Server - player functions */
struct player *get_player_by_ids(struct server *s, uint32_t pub_id, uint32_t priv_id);
struct player *get_player_by_public_id(struct server *s, uint32_t pub_id);
int add_player(struct server *serv, struct player *pl);
void remove_player(struct server *s, struct player *p);
int move_player(struct player *p, struct channel *to);

/* Server - ban functions */
int add_ban(struct server *s, struct ban *b);
void remove_ban(struct server *s, struct ban *b);
struct ban *get_ban_by_id(struct server *s, uint16_t id);
struct ban *get_ban_by_ip(struct server *s, struct in_addr ip);

/* Server - registration functions */
struct registration *get_registration(struct server *s, char *login, char *pass);
int add_registration(struct server *s, struct registration *r);

void print_server(struct server *s);

void server_start(struct server *s);
#endif
