#include "channel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int destroy_channel(/*struct server * chanman, */struct channel * chan)
{
	/* TODO : relocate players to another channel maybe? */
	
	free(chan);
	return 1;
}

/** Initialize a new channel */
struct channel * 
new_channel(char * name, char * topic, char * desc, uint16_t flags, uint16_t codec, uint16_t sort_order, uint16_t max_users)
{
	struct channel * chan;
	chan = (struct channel *)malloc(sizeof(struct channel));
	
	chan->current_users = 0;
	chan->players = (struct player **)malloc(sizeof(struct player *) * max_users);
	
	chan->name = strdup(name);
	chan->topic = strdup(topic);
	chan->desc = strdup(desc);

	chan->flags = flags;
	chan->codec = codec;
	chan->sort_order = sort_order;
	chan->max_users = max_users;
	
	return chan;
};

/** Initialize a default channel for testing */
struct channel * new_predef_channel()
{
	uint16_t flags = CHANNEL_FLAG_REGISTERED;
	return new_channel("Channel name", "Channel topic", "Channel description", flags, CODEC_SPEEX_19_6, 0, 16);
}

void print_channel(struct channel * chan)
{
	if(chan == NULL) {
		printf("Channel NULL\n");
	} else {
		printf("Channel ID %i\n", chan->id);
		printf("\t name    : %s\n", chan->name);
		printf("\t topic   : %s\n", chan->topic);
		printf("\t desc    : %s\n", chan->desc);
		if(chan->flags & CHANNEL_FLAG_DEFAULT)
			printf("\t default : y\n");
	}
}

int add_player_to_channel(struct channel * chan, struct player * pl)
{
	int i;
	
	pl->in_chan = chan;
	chan->current_users++;
	
	for(i=0 ; i<chan->max_users ; i++) {
		if(chan->players[i] == NULL) {
			chan->players[i] = pl;
		}
		return 1;
	}
	return 0;
}
