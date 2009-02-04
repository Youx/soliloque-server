#include "channel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Destroys a channel and all its fields
 *
 * @param chan the channel we are going to destroy
 *
 * @return 1 on success
 */
int destroy_channel(struct channel *chan)
{
	/* TODO : relocate players to another channel maybe? */
	free(chan->name);
	free(chan->topic);
	free(chan->desc);
	free(chan->players);

	free(chan);
	return 1;
}

/**
 * Initialize a new channel.
 * 
 * @param name the name of the channel
 * @param topic the topic of the channel
 * @param desc the description of the channel
 * @param flags the flags of the channel in a bitfield (see channel.h for values)
 * @param codec the codec of the channel (see channel.h for values)
 * @param sort_order the sort order of the channel. No idea what it is
 * @param max_users the maximum number of users in the channel
 *
 * @return the newly allocated and initialized channel
 */
struct channel *new_channel(char *name, char *topic, char *desc, uint16_t flags,
		uint16_t codec, uint16_t sort_order, uint16_t max_users)
{
	struct channel *chan;
	chan = (struct channel *)calloc(1, sizeof(struct channel));
	
	bzero(chan->password, 30);
	chan->current_users = 0;
	chan->players = (struct player **)calloc(max_users, sizeof(struct player *));
	
	chan->name = strdup(name);
	chan->topic = strdup(topic);
	chan->desc = strdup(desc);

	chan->flags = flags;
	chan->codec = codec;
	chan->sort_order = sort_order;
	chan->max_users = max_users;
	
	return chan;
};

/**
 * Initialize a default channel for testing.
 * This will not give a channel it's ID (determined at insertion)
 *
 * @return a test channel.
 */
struct channel *new_predef_channel()
{
	uint16_t flags = CHANNEL_FLAG_REGISTERED;
	return new_channel("Channel name", "Channel topic", "Channel description", flags, CODEC_SPEEX_19_6, 0, 16);
}


/**
 * Prints a channel's information.
 * @param chan a channel
 */
void print_channel(struct channel *chan)
{
	if (chan == NULL) {
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

/**
 * Add a player to a channel (both have to exist).
 * This will NOT remove the player from his existing channel if
 * he has one.
 * @param chan an existing channel
 * @param pl an existing player
 *
 * @return 1 if the insertion succeeded, 
 * 	   0 if it failed (maximum number of users in channel already reached
 */
int add_player_to_channel(struct channel *chan, struct player *pl)
{
	int i;
	
	pl->in_chan = chan;
	chan->current_users++;
	
	for (i = 0 ; i < chan->max_users ; i++) {
		if (chan->players[i] == NULL) {
			chan->players[i] = pl;
			return 1;
		}
	}
	chan->current_users--;
	pl->in_chan = NULL;
	return 0;
}

/**
 * Converts a channel to a data block that can be sent
 * over the network.
 *
 * @param ch the channel we are going to convert to a data block
 * @param data a pointer to an already allocated data block
 *
 * @return the number of bytes that have been written
 */
int channel_to_data(struct channel *ch, char *data)
{
	int size = 4 + 2 + 2 + 4 + 2 + 2
	        + strlen(ch->name) + 1
		+ strlen(ch->topic) + 1
		+ strlen(ch->desc) + 1;
	char *ptr;

	ptr = data;

	*(uint32_t *)ptr = ch->id;		ptr += 4;
	*(uint16_t *)ptr = ch->flags;		ptr += 2;
	*(uint16_t *)ptr = ch->codec;		ptr += 2;
	*(uint32_t *)ptr = 0xFFFFFFFF;		ptr += 4;
	*(uint16_t *)ptr = ch->sort_order;	ptr += 2;
	*(uint16_t *)ptr = ch->max_users;	ptr += 2;
	strcpy(ptr, ch->name);			ptr += (strlen(ch->name) +1);
	strcpy(ptr, ch->topic);			ptr += (strlen(ch->topic) +1);
	strcpy(ptr, ch->desc);			ptr += (strlen(ch->desc) +1);

	return size;
}

/**
 * Compute the size the channel will take once converted
 * to raw data
 *
 * @param ch the channel
 *
 * @return the size the channel will take
 */
int channel_to_data_size(struct channel *ch)
{
	return 4 + 2 + 2 + 4 + 2 + 2
	        + strlen(ch->name) + 1
		+ strlen(ch->topic) + 1
		+ strlen(ch->desc) + 1;
}


size_t channel_from_data(char *data, int len, struct channel **dst)
{
	uint16_t flags;
	uint16_t codec;
	uint16_t sort_order;
	uint16_t max_users;
	char *name, *topic, *desc, *ptr;

	ptr = data;

	/* ignore ID field */			ptr += 4;
	flags = *(uint16_t *)ptr;		ptr += 2;
	codec = *(uint16_t *)ptr;		ptr += 2;
	/* ignore 0xFFFFFFFF field */		ptr += 4;
	sort_order = *(uint16_t *)ptr;		ptr += 2;
	max_users = *(uint16_t *)ptr;		ptr += 2;
	name = strdup(ptr);			ptr += strlen(name) + 1;
	topic = strdup(ptr);			ptr += strlen(topic) + 1;
	desc = strdup(ptr);			ptr += strlen(desc) + 1;

	*dst = new_channel(name, topic, desc, flags, codec, sort_order, max_users);
	return ptr - data;
}
