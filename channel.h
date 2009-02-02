#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "player.h"
#include "server.h"
#include "compat.h"
#include "audio_packet.h"


#define CHANNEL_FLAG_REGISTERED   0
#define CHANNEL_FLAG_UNREGISTERED 1
#define CHANNEL_FLAG_MODERATED    2
#define CHANNEL_FLAG_PASSWORD   4
#define CHANNEL_FLAG_SUBCHANNELS  8
#define CHANNEL_FLAG_DEFAULT    16


struct channel {
	uint32_t id;
	uint16_t flags;
	uint8_t codec;
	uint16_t sort_order;
	uint16_t max_users;
	
	char *name;
	char *topic;
	char *desc;
	char password[30];

	uint16_t current_users;
	struct player **players;
	struct server *in_server;

	uint32_t db_id;
};



/* Channel functions */
struct channel *new_channel(char *name, char *topic, char *desc, uint16_t flags,
		uint16_t codec, uint16_t sort_order, uint16_t max_users);
struct channel *new_predef_channel();
int destroy_channel(struct channel *chan);

int add_player_to_channel(struct channel *chan, struct player *player);

void print_channel(struct channel *chan);

int channel_to_data(struct channel *ch, char *data);
int channel_to_data_size(struct channel *ch);

#endif
