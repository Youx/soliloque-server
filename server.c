#include "server.h"
#include "channel.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Create and initialize a new server
 * The default number of channels if 4, 
 * the default number of players is 8.
 *
 * @return the new server
 */
struct server *new_server()
{
	struct server *serv;
	serv = (struct server *)malloc(sizeof(struct server));
	
	serv->chans = ar_new(4);
	serv->players = ar_new(8);
	serv->bans = ar_new(4);
	
	return serv;
}

/**
 * Add a channel to the channel list
 *
 * @param serv the server
 * @param chan the channel
 *
 * @return 1 if insertion succeeded, or 0 if it failed.
 *
 * TODO: add the network callback to notify all players a channel has been added.
 */
int add_channel(struct server *serv, struct channel *chan)
{
	uint32_t new_id;
	struct channel *tmp_chan;
	char *used_ids;
	
	/* If there is no channel, make this channel the default one */
	if (serv->chans->used_slots == 0)
		chan->flags |= (CHANNEL_FLAG_DEFAULT | CHANNEL_FLAG_REGISTERED);
		
	/* If this channel is the default one, remove the flag from the channel that previously has it.
	 *  = Only one channel can have the default flag */
	if (chan->flags & CHANNEL_FLAG_DEFAULT) {
		ar_each(struct channel *, tmp_chan, serv->chans)
			tmp_chan->flags &= (0xFFFF ^ CHANNEL_FLAG_DEFAULT);
		ar_end_each;
	}
	
	/* Find the next available ID */
	used_ids = (char *)calloc(serv->chans->total_slots, sizeof(char));
	ar_each(struct channel *, tmp_chan, serv->chans)
		used_ids[tmp_chan->id - 1] = 1;	/* id -1  -> ID start at 1 */
	ar_end_each;

	new_id = 0;
	while(used_ids[new_id] == 1)
		new_id++;
	new_id += 1;	/* ID start at 1 */
	
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
 *
 * @param serv the server
 * @param id the ID of the channel
 *
 * @return the channel we retrieved, a NULL pointer if it failed.
 */
struct channel *get_channel_by_id(struct server *serv, uint32_t id)
{
	struct channel *tmp_chan;
	
	ar_each(struct channel *, tmp_chan, serv->chans)
		if(tmp_chan->id == id)
			return tmp_chan;
	ar_end_each;

	/* channel not found */
	return NULL;
}

/**
 * Destroys a channel given it's ID
 *
 * @param serv the server
 * @param id the id of the channel we want to destroy
 *
 * @return 1 on success, 0 if the channel wasn't found
 */
int destroy_channel_by_id(struct server *serv, uint32_t id)
{
	struct channel *tmp_chan;
	
	ar_each(struct channel *, tmp_chan, serv->chans)
		if(tmp_chan->id == id) {
			destroy_channel(tmp_chan);
			ar_remove(serv->chans, tmp_chan);
			return 1;
		}
	ar_end_each;

	return 0;
}

/**
 * Returns the default channel of the server.
 * If there is no default channel, we create one and register it
 *
 * @param serv the server
 *
 * @return the default channel of the server
 */
struct channel *get_default_channel(struct server *serv)
{
	struct channel *new_chan, *tmp_chan;
	
	ar_each(struct channel *, tmp_chan, serv->chans)
		if (tmp_chan->flags & CHANNEL_FLAG_DEFAULT)
			return tmp_chan;
	ar_end_each;

	/* If no default channel exists, we create one ! */
	new_chan =  new_channel("Default", "Default channel", "This is the default channel", 
		CHANNEL_FLAG_DEFAULT | CHANNEL_FLAG_REGISTERED, CODEC_SPEEX_16_3, 0, 16);
	add_channel(serv, new_chan);
	return new_chan;
}

/**
 * Add a player to the server and put it into the default channel.
 *
 * @param serv the server
 * @param pl the player
 *
 * @return 1 if the insertion suceeded, 0 if it failed
 */
int add_player(struct server *serv, struct player *pl)
{
	struct channel *def_chan;
	char *used_ids;
	int new_id;
	struct player *tmp_pl;
	
	def_chan = get_default_channel(serv);
	
	/* Find the next available public ID */
	used_ids = (char *)calloc(serv->players->total_slots, sizeof(char));
	ar_each(struct player *, tmp_pl, serv->players)
		used_ids[tmp_pl->public_id - 1] = 1;	/* ID start at 1 */
	ar_end_each;

	new_id = 0;
	while (used_ids[new_id] == 1)
		new_id++;
	pl->public_id = new_id + 1;	/* ID start at 1 */

	/* Find the next available private ID */
	pl->private_id = random();
	/* Find next slot in the array */
	ar_insert(serv->players, pl);

	return add_player_to_channel(def_chan, pl);
}

/**
 * Retrieve a player with its public and private ids.
 *
 * @param s the server
 * @param pub_id the public id of the player
 * @param priv_id the private id of the player
 *
 * @return the player if it was found (both ids have to be valid), a NULL pointer if it failed.
 */
struct player *get_player_by_ids(struct server *s, uint32_t pub_id, uint32_t priv_id)
{
	struct player *pl;

	ar_each(struct player *, pl, s->players)
		if (pl->public_id == pub_id && pl->private_id == priv_id) {
			return pl;
		}
	ar_end_each;

	return NULL;
}

/**
 * Retrieve a player with its public id.
 *
 * @param s the server
 * @param pub_id the public id of the player
 *
 * @return the player if it was found, a NULL pointer if it failed.
 */
struct player *get_player_by_public_id(struct server *s, uint32_t pub_id)
{
	struct player *pl;

	ar_each(struct player *, pl, s->players)
		if (pl->public_id == pub_id) {
			return pl;
		}
	ar_end_each;

	return NULL;
}

/**
 * Remove a player from the server & channels.
 *
 * @param s the server we will remove the player from
 * @param p the player that will be removed
 */
void remove_player(struct server *s, struct player *p)
{
	int i;

	/* remove from the server */
	ar_remove(s->players, (void *)p);
	/* remove from the channel */
	for (i=0 ; i < p->in_chan->max_users ; i++) {
		if (p->in_chan->players[i] == p)
			p->in_chan->players[i] = NULL;
	}
	p->in_chan->current_users--;
	/* free the memory */
	destroy_player(p);
}

/**
 * Move a player from its current channel to another.
 *
 * @param p the player we are moving
 * @param to the channel we are moving him to
 */
int move_player(struct player *p, struct channel *to)
{
	struct channel *old;
	int i;

	if (p->in_chan == NULL) {
		return add_player_to_channel(to, p);
	}

	if (to->current_users >= to->max_users)
		return 0;

	/* remove from the old channel */
	old = p->in_chan;
	for(i = 0 ; i < old->max_users ; i++) {
		if(old->players[i] == p)
			old->players[i] = NULL;
	}
	old->current_users--;
	/* put inside the new channel */
	p->in_chan = to;
	for(i = 0 ; i < to->max_users ; i++) {
		if (to->players[i] == NULL)
			to->players[i] = p;
	}
	to->current_users++;
	return 1;
}

/**
 * Add a new ban to the server.
 *
 * @param s the server
 * @param b the ban
 *
 * @return 1 on success
 */
int add_ban(struct server *s, struct ban *b)
{
	struct ban *tmp_b;
	char *used_ids;
	int new_id;

	/* Find the next available public ID */
	used_ids = (char *)calloc(s->bans->total_slots, sizeof(char));
	ar_each(struct ban *, tmp_b, s->bans)
		used_ids[tmp_b->id - 1] = 1;	/* ID start at 1 */
	ar_end_each;

	new_id = 0;
	while (used_ids[new_id] == 1)
		new_id++;
	b->id = new_id + 1;	/* ID start at 1 */
	free(used_ids);

	ar_insert(s->bans, (void *)b);
	return 1;
}

/**
 * Retrieves a ban with its ID.
 *
 * @param s the server
 * @param ban_id the id of the ban
 *
 * @return the ban, or NULL if it does not exist
 */
struct ban *get_ban_by_id(struct server *s, uint16_t ban_id)
{
	struct ban *b;

	ar_each(struct ban *, b, s->bans)
		if(b->id == ban_id)
			return b;
	ar_end_each;

	return NULL;
}

/**
 * Retrieves a ban with its IP if it exists.
 *
 * @param s the server
 * @param ip the ip of the player
 *
 * @return the ban, or NULL if it does not exist
 */
struct ban *get_ban_by_ip(struct server *s, struct in_addr ip)
{
	struct ban *b;

	ar_each(struct ban *, b, s->bans)
		if (strcmp(b->ip, inet_ntoa(ip)) == 0)
			return b;
	ar_end_each;

	return NULL;
}

/**
 * Removes a ban from the server.
 *
 * @param s the server
 * @param b the ban we want to remove
 *
 * @return
 */
void remove_ban(struct server *s, struct ban *b)
{
	ar_remove(s->bans, (void *)b);
}

/**
 * Prints information about the server (channels, etc)
 *
 * @param s the server
 */
void print_server(struct server *s)
{
	struct channel *tmp_chan;
	struct player *tmp_pl;

	ar_each(struct channel *, tmp_chan, s->chans)
		print_channel(tmp_chan);
	ar_end_each;

	ar_each(struct player *, tmp_pl, s->players)
		print_player(tmp_pl);
	ar_end_each;
}
