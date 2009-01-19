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
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the players.
 *
 * @param pl the player we send the player list to
 * @param s the server we will get the players from
 *
 * TODO : split the players over packets
 */
void s_resp_players(struct player *pl, struct server *s)
{
	char *data;
	int data_size = 0;
	struct player *p;
	char *ptr;
	int p_size;
	/* compute the size of the packet */
	data_size += 24;	/* header */
	data_size += 4;		/* number of players in packet */
	ar_each(struct player *, p, s->players)
		data_size += player_to_data_size(p);
	ar_end_each;

	/* initialize the packet */
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;
	*(uint32_t *)ptr = 0x0007bef0;			ptr+=4;
	*(uint32_t *)ptr = pl->private_id;		ptr+=4;		/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr+=4;		/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr+=4;		/* packet counter */
	/* packet version */				ptr+=4;
	/* empty checksum */				ptr+=4;
	*(uint32_t *)ptr = s->players->used_slots;	ptr+=4;
	/* dump the players to the packet */
	ar_each(struct player *, p, s->players)
		p_size = player_to_data_size(p);
		player_to_data(p, ptr);
		ptr+=p_size;
	ar_end_each;

	packet_add_crc_d(data, data_size);

	printf("size of all players : %i\n", data_size);
	sendto(socket_desc, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f0_s_counter++;
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
