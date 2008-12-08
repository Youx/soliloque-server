#include "server.h"
#include "channel.h"
#include <stdlib.h>

/**
 * Create and initialize a new server
 */
struct server * new_server()
{
	struct server * serv;
	serv = (struct server *)malloc(sizeof(struct server));
	
	serv->chans = ar_new(4);
	serv->players = ar_new(8);
	
	return serv;
}

/**
 * Add a channel to the channel list
 */
int add_channel(struct server * serv, struct channel * chan)
{
	uint32_t new_id;
	struct channel *tmp_chan;
	
	char * used_ids;
	
	/* If there is no channel, make this channel the default one */
	if(serv->chans->used_slots == 0)
		chan->flags |= (CHANNEL_FLAG_DEFAULT | CHANNEL_FLAG_REGISTERED);
		
	/* If this channel is the default one, remove the flag from the channel that previously has it.
	 *  = Only one channel can have the default flag */
	if(chan->flags & CHANNEL_FLAG_DEFAULT) {
		ar_each(struct channel *, tmp_chan, serv->chans)
			tmp_chan->flags &= (0xFFFF ^ CHANNEL_FLAG_DEFAULT);
		ar_end_each
	}
	
	/* Find the next available ID */
	used_ids = (char *)calloc(serv->chans->total_slots, sizeof(char));
	ar_each(struct channel *, tmp_chan, serv->chans)
		used_ids[tmp_chan->id] = 1;
	ar_end_each

	new_id = 0;
	while(used_ids[new_id] == 1)
		new_id++;
	
	/* set ID and insert into that slot */
	chan->id = new_id;
	ar_insert(serv->chans, chan);
	chan->in_server = serv;
	/* NETWORK CALLBACK */
	
	/* END OF NETWORK CALLBACK */
	
	free(used_ids);
	
	return 1;
}


/**
 * Retrieve a channel with a given ID
 */
struct channel * get_channel_by_id(struct server * serv, uint32_t id)
{
	struct channel * tmp_chan;
	
	ar_each(struct channel *, tmp_chan, serv->chans)
		if(tmp_chan->id == id)
			return tmp_chan;
	ar_end_each

	/* channel not found */
	return NULL;
}

/**
 * Destroys a channel given it's ID
 */
int destroy_channel_by_id(struct server * serv, uint32_t id)
{
	struct channel * tmp_chan;
	
	ar_each(struct channel *, tmp_chan, serv->chans)
		if(tmp_chan->id == id) {
			destroy_channel(tmp_chan);
			ar_remove(serv->chans, tmp_chan);
			return 1;
		}
	ar_end_each

	return 0;
}

/**
 * Returns the default channel of the server.
 * If there is no default channel, we create one and register it
 */
struct channel * get_default_channel(struct server * serv)
{
	struct channel *new_chan, *tmp_chan;
	
	ar_each(struct channel *, tmp_chan, serv->chans)
		if(tmp_chan->flags & CHANNEL_FLAG_DEFAULT != 0)
			return tmp_chan;
	ar_end_each

	/* If no default channel exists, we create one ! */
	new_chan =  new_channel("Default", "Default channel", "This is the default channel", 
		CHANNEL_FLAG_DEFAULT | CHANNEL_FLAG_REGISTERED, CODEC_SPEEX_16_3, 0, 16);
	add_channel(serv, new_chan);
	return new_chan;
}

int add_player(struct server * serv, struct player * pl)
{
	struct channel * def_chan;
	char * used_ids;
	int new_id;
	struct channel * tmp_chan;
	
	def_chan = get_default_channel(serv);
	
	/* Find the next available public ID */
	used_ids = (char *)calloc(serv->players->total_slots, sizeof(char));
	ar_each(struct channel *, tmp_chan, serv->chans)
		used_ids[tmp_chan->id] = 1;
	ar_end_each

	new_id = 0;
	while(used_ids[new_id] == 1)
		new_id++;
	pl->public_id = new_id;
	
	/* Find the next available private ID */
	pl->private_id = random();
	
	/* find next slot in the array */
	ar_insert(serv->players, pl);
	
	return add_player_to_channel(def_chan, pl);
}


/**
 * Prints information about the server (channels, etc)
 */
void print_server(struct server * s)
{
	struct channel * tmp_chan;
	
	ar_each(struct channel *, tmp_chan, s->chans)
		print_channel(tmp_chan);
	ar_end_each
}