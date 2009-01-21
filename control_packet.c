#include "control_packet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"
#include "channel.h"
#include "acknowledge_packet.h"
#include "array.h"
#include "packet_tools.h"

extern struct server *ts_server;
extern int socket_desc;
/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the channels.
 *
 * @param pl the player we send the channel list to
 * @param s the server we will get the channels from
 *
 * TODO : split the channels over packets
 */
void s_resp_chans(struct player *pl, struct server *s)
{
	char *data;
	int data_size = 0;
	struct channel *ch;
	char *ptr;
	int ch_size;
	/* compute the size of the packet */
	data_size += 24;	/* header */
	data_size += 4;		/* number of channels in packet */
	ar_each(struct channel *, ch, s->chans)
		data_size += channel_to_data_size(ch);
	ar_end_each;

	/* initialize the packet */
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;
	*(uint32_t *)ptr = 0x0006bef0;			ptr+=4;		/* */
	*(uint32_t *)ptr = pl->private_id;		ptr+=4;		/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr+=4;		/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr+=4;		/* packet counter */
	/* packet version */				ptr+=4;
	/* empty checksum */				ptr+=4;
	*(uint32_t *)ptr = s->chans->used_slots;	ptr+=4;		/* number of channels sent */
	/* dump the channels to the packet */
	ar_each(struct channel *, ch, s->chans)
		ch_size = channel_to_data_size(ch);
		channel_to_data(ch, ptr);
		ptr+=ch_size;
	ar_end_each;

	packet_add_crc_d(data, data_size);

	printf("size of all channels : %i\n", data_size);
	sendto(socket_desc, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Notify all players on the server that a new player arrived.
 *
 * @param pl the player who arrived
 * @param s the server
 */
void s_notify_new_player(struct player *pl, struct server *s)
{
	struct player *tmp_pl;
	char *data, *ptr;
	int data_size;

	data_size = 24+player_to_data_size(pl);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0064bef0;		ptr+=4;
	/* public and private ID */		ptr+=8;	/* done later */
	/* counter */				ptr+=4;	/* done later */
	/* packet version */			ptr+=4; /* empty for now */
	/* empty checksum */			ptr+=4; /* done later */
	player_to_data(pl, ptr);
	
	/* customize and send for each player on the server */
	ar_each(struct player *, tmp_pl, s->players)
		if(tmp_pl != pl) {
			*(uint32_t *)(data+4) = tmp_pl->private_id;
			*(uint32_t *)(data+8) = tmp_pl->public_id;
			*(uint32_t *)(data+12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0, (struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
		}
	ar_end_each;

	free(data);
}



/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the players.
 *
 * @param pl the player we send the player list to
 * @param s the server we will get the players from
 *
 * TODO : split the players over packets (max 10 par paquets)
 */
void s_resp_players(struct player *pl, struct server *s)
{
	char *data;
	int data_size = 0;
	char *ptr;
	int p_size;
	int nb_players;
	struct player *pls[10];
	int i;
	int players_copied;
	/* compute the size of the packet */
	data_size += 24;	/* header */
	data_size += 4;		/* number of players in packet */
	data_size += 10*player_to_data_size(NULL); /* players */

	nb_players = s->players->used_slots;
	data = (char *)calloc(data_size, sizeof(char));
	while(nb_players > 0) {
		bzero(data, data_size * sizeof(char));
		ptr = data;
		/* initialize the packet */
		*(uint32_t *)ptr = 0x0007bef0;				ptr+=4;
		*(uint32_t *)ptr = pl->private_id;			ptr+=4;		/* player private id */
		*(uint32_t *)ptr = pl->public_id;			ptr+=4;		/* player public id */
		*(uint32_t *)ptr = pl->f0_s_counter;			ptr+=4;		/* packet counter */
		/* packet version */					ptr+=4;
		/* empty checksum */					ptr+=4;
		*(uint32_t *)ptr = MIN(10, nb_players);			ptr+=4;
		/* dump the players to the packet */
		bzero(pls, 10*sizeof(struct player *));
		players_copied = ar_get_n_elems_start_at(s->players, 10, s->players->used_slots-nb_players, (void**)pls);
		for(i=0 ; i< players_copied; i++) {
			p_size = player_to_data_size(pls[i]);
			player_to_data(pls[i], ptr);
			ptr+=p_size;
		}
		packet_add_crc_d(data, data_size);

		printf("size of all players : %i\n", data_size);
		sendto(socket_desc, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
		pl->f0_s_counter++;
		/* decrement the number of players to send */
		nb_players -= MIN(10, nb_players);
	}
	free(data);
}

void s_resp_unknown(struct player *pl, struct server *s)
{
	char *data;
	char *ptr;
	int data_size = 283;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;
	/* initialize the packet */
	*(uint32_t *)ptr = 0x0008bef0;			ptr+=4;
	*(uint32_t *)ptr = pl->private_id;		ptr+=4;		/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr+=4;		/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr+=4;		/* packet counter */
	/* packet version */				ptr+=4;
	/* empty checksum */				ptr+=4;
	/* empty data ??? */				ptr+=256;
	*ptr = 0x6e;					ptr++;
	*ptr = 0x61;					ptr++;
							ptr++;

	packet_add_crc_d(data, data_size);

	sendto(socket_desc, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Handles a request for channels and players by the player.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the client
 * @param cli_len the size of cli_addr
 */
void *c_req_chans(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id;
	struct player *pl;

	memcpy(&priv_id, data+4, 4);
	memcpy(&pub_id, data+8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);

	if(pl != NULL) {
		send_acknowledge(pl);		/* ACK */
		s_resp_chans(pl, ts_server);	/* list of channels */
		usleep(250000);
		s_resp_players(pl, ts_server);	/* list of players */
		usleep(250000);
		s_resp_unknown(pl, ts_server);
	}
	return NULL;
}
