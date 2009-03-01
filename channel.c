#include "channel.h"
#include "array.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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
	ar_free(chan->players);

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
	if (chan == NULL) {
		printf("(EE) new_channel, calloc failed : %s.\n", strerror(errno));
		return NULL;
	}
	
	bzero(chan->password, 30);
	chan->players = ar_new(4);
	chan->players->max_slots = max_users;
	/* subchannels */
	chan->subchannels = ar_new(4);
	chan->parent = NULL;
	chan->parent_db_id = 0;

	/* strdup : input strings are secure */
	chan->name = strdup(name);
	chan->topic = strdup(topic);
	chan->desc = strdup(desc);

	chan->flags = flags;
	chan->codec = codec;
	chan->sort_order = sort_order;

	if (chan->name == NULL || chan->topic == NULL || chan->desc == NULL) {
		if (chan->name != NULL)
			free(chan->name);
		if (chan->topic != NULL)
			free(chan->topic);
		if (chan->desc != NULL)
			free(chan->desc);
		free(chan);
		return NULL;
	}
	
	return chan;
}

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
	if (ar_insert(chan->players, pl) == AR_OK) {
		pl->in_chan = chan;
		return 1;
	}
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
	if (ch->parent == NULL) {
		*(uint32_t *)ptr = 0xFFFFFFFF;		ptr += 4;
	} else {
		*(uint32_t *)ptr = ch->parent->id;	ptr += 4;
	}
	*(uint16_t *)ptr = ch->sort_order;	ptr += 2;
	*(uint16_t *)ptr = ch->players->max_slots;	ptr += 2;
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
	uint32_t parent_id;
	char *name, *topic, *desc, *ptr;

	ptr = data;

	/* ignore ID field */			ptr += 4;
	flags = *(uint16_t *)ptr;		ptr += 2;
	codec = *(uint16_t *)ptr;		ptr += 2;
	parent_id = *(uint32_t *)ptr;		ptr += 4;
	sort_order = *(uint16_t *)ptr;		ptr += 2;
	max_users = *(uint16_t *)ptr;		ptr += 2;
	/* TODO : check if the len - (ptr - data) - X formula is correct */
	name = strndup(ptr, len - (ptr - data) - 3);		ptr += strlen(name) + 1;
	topic = strndup(ptr, len - (ptr - data) - 2);		ptr += strlen(topic) + 1;
	desc = strndup(ptr, len - (ptr - data) - 1);		ptr += strlen(desc) + 1;

	if (name == NULL || topic == NULL || desc == NULL) {
		if (name != NULL)
			free(name);
		if (topic != NULL)
			free(topic);
		if (desc != NULL)
			free(desc);
		printf("(WW) channel_from_data, allocation failed : %s.\n", strerror(errno));
		return 0;
	}
	*dst = new_channel(name, topic, desc, flags, codec, sort_order, max_users);

	if (parent_id != 0xFFFFFFFF)
		(*dst)->parent_id = parent_id;

	return ptr - data;
}

int channel_remove_subchannel(struct channel *ch, struct channel *subchannel)
{
	if (ch != subchannel->parent) {
		printf("(WW) channel_remove_subchannel, subchannel's parent is not the same as passed parent.\n");
		return 0;
	}
	if (ch == NULL) {
		printf("(WW) channel_remove_subchannel, parent is null.\n");
		return 0;
	}
	ar_remove(ch->subchannels, subchannel);
	subchannel->parent = NULL;

	return 1;
}

int channel_add_subchannel(struct channel *ch, struct channel *subchannel)
{
	if ((ch->flags & CHANNEL_FLAG_SUBCHANNELS) == 0) {
		printf("(WW) channel_add_subchannel, channel %i:%s can not have subchannels.\n", ch->id, ch->name);
		return 0;
	}
	channel_remove_subchannel(subchannel->parent, subchannel);
	subchannel->parent = ch;
	ar_insert(ch->subchannels, subchannel);
	return 1;
}
