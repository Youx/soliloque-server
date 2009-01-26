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
	*(uint32_t *)ptr = 0x0006bef0;			ptr += 4;	/* */
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;	/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	*(uint32_t *)ptr = s->chans->used_slots;	ptr += 4;	/* number of channels sent */
	/* dump the channels to the packet */
	ar_each(struct channel *, ch, s->chans)
		ch_size = channel_to_data_size(ch);
		channel_to_data(ch, ptr);
		ptr += ch_size;
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

	data_size = 24 + player_to_data_size(pl);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0064bef0;		ptr += 4;
	/* public and private ID */		ptr += 8;	/* done later */
	/* counter */				ptr += 4;	/* done later */
	/* packet version */			ptr += 4;	/* empty for now */
	/* empty checksum */			ptr += 4;	/* done later */
	player_to_data(pl, ptr);
	
	/* customize and send for each player on the server */
	ar_each(struct player *, tmp_pl, s->players)
		if (tmp_pl != pl) {
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0, (struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
		}
	ar_end_each;

	free(data);
}

/**
 * Send a "player disconnected" message to all players.
 *
 * @param s the server
 * @param p the player who left
 */
void s_notify_player_left(struct server *s, struct player *p)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0065bef0;		ptr += 4;	/* function code */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	*(uint32_t *)ptr = p->f0_s_counter;	ptr += 4;	/* packet counter */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = p->public_id;	ptr += 4;	/* ID of player who left */
	*(uint32_t *)ptr = 1;			ptr += 4;	/* visible notification */
	/* 32 bytes of garbage?? */		ptr += 32;	/* maybe some message ? */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the players.
 *
 * @param pl the player we send the player list to
 * @param s the server we will get the players from
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
	data_size += 10 * player_to_data_size(NULL); /* players */

	nb_players = s->players->used_slots;
	data = (char *)calloc(data_size, sizeof(char));
	while (nb_players > 0) {
		bzero(data, data_size * sizeof(char));
		ptr = data;
		/* initialize the packet */
		*(uint32_t *)ptr = 0x0007bef0;			ptr += 4;
		*(uint32_t *)ptr = pl->private_id;		ptr += 4;	/* player private id */
		*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* player public id */
		*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;	/* packet counter */
		/* packet version */				ptr += 4;
		/* empty checksum */				ptr += 4;
		*(uint32_t *)ptr = MIN(10, nb_players);		ptr += 4;
		/* dump the players to the packet */
		bzero(pls, 10 * sizeof(struct player *));
		players_copied = ar_get_n_elems_start_at(s->players, 10,
				s->players->used_slots-nb_players, (void**)pls);
		for (i = 0 ; i < players_copied ; i++) {
			p_size = player_to_data_size(pls[i]);
			player_to_data(pls[i], ptr);
			ptr += p_size;
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
	*(uint32_t *)ptr = 0x0008bef0;			ptr += 4;
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;		/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;		/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;		/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	/* empty data ??? */				ptr += 256;
	*ptr = 0x6e;					ptr += 1;
	*ptr = 0x61;					ptr += 1;
							ptr += 1;

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

	if (pl != NULL) {
		send_acknowledge(pl);		/* ACK */
		s_resp_chans(pl, ts_server);	/* list of channels */
		usleep(250000);
		s_resp_players(pl, ts_server);	/* list of players */
		usleep(250000);
		s_resp_unknown(pl, ts_server);
	}
	return NULL;
}

/**
 * Handles a disconnection request.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the client
 * @param cli_len the size of cli_addr
 * // F0 BE 2C 01
 */
void *c_req_leave(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id;
	struct player *pl;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);
	if (pl != NULL) {
		send_acknowledge(pl);		/* ACK */
		/* send a notification to all players */
		s_notify_player_left(ts_server, pl);
		remove_player(ts_server, pl);
	}
	return NULL;
}


/**
 * Send a "player kicked" notification to all players.
 *
 * @param s the server
 * @param kicker the player who kicked
 * @param kicked the player kicked from the server
 * @param reason the reason the player was kicked
 */
void s_notify_kick_server(struct server *s, struct player *kicker, struct player *kicked, char *reason)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0065bef0;		ptr += 4;	/* function code */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = kicked->public_id;	ptr += 4;	/* ID of player who left */
	*(uint16_t *)ptr = 2;			ptr += 2;	/* visible notification : kicked */
	*(uint32_t *)ptr = kicker->public_id;	ptr += 4;	/* kicker ID */
	*(uint8_t *)ptr = strlen(reason);	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, strlen(reason));	ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handles a server kick request.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_kick_server(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id, target_id;
	char *reason;
	struct player *pl, *target;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);
	memcpy(&target_id, data + 24, 4);
	target = get_player_by_public_id(ts_server, target_id);

	if (pl != NULL && target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(pl->global_flags & GLOBAL_FLAG_SERVERADMIN) {
			reason = strndup(data + 29, MIN(29, data[28]));
			printf("Reason for kicking player %s : %s\n", target->name, reason);
			s_notify_kick_server(ts_server, pl, target, reason);
			remove_player(ts_server, pl);
			free(reason);
		}
	}

	return NULL;
}

/**
 * Send a "player kicked from channel" notification to all players.
 *
 * @param s the server
 * @param kicker the player who kicked
 * @param kicked the player kicked from the server
 * @param reason the reason the player was kicked
 */
void s_notify_kick_channel(struct server *s, struct player *kicker, struct player *kicked, 
		char *reason, struct channel *kicked_to)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 68;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0066bef0;			ptr += 4;	/* function code */
	/* private ID */				ptr += 4;	/* filled later */
	/* public ID */					ptr += 4;	/* filled later */
	/* packet counter */				ptr += 4;	/* filled later */
	/* packet version */				ptr += 4;	/* not done yet */
	/* empty checksum */				ptr += 4;	/* filled later */
	*(uint32_t *)ptr = kicked->public_id;		ptr += 4;	/* ID of player who left */
	*(uint32_t *)ptr = kicker->public_id;		ptr += 4;	/* kicker ID */
	*(uint32_t *)ptr = kicked_to->id;		ptr += 4;	/* channel the player is moved to */
	*(uint16_t *)ptr = 0;				ptr += 2;	/* visible notification : kicked */
	*(uint8_t *)ptr = (uint8_t)strlen(reason);	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, strlen(reason));		ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handles a channel kick request.
 */
void *c_req_kick_channel(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id, target_id;
	char *reason;
	struct player *pl, *target;
	struct channel *def_chan;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);
	memcpy(&target_id, data + 24, 4);
	target = get_player_by_public_id(ts_server, target_id);
	def_chan = get_default_channel(ts_server);

	if (pl != NULL && target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if ((pl->chan_privileges & CHANNEL_PRIV_CHANADMIN || pl->global_flags & GLOBAL_FLAG_SERVERADMIN) &&
				pl->in_chan == target->in_chan) {
			reason = strndup(data + 29, MIN(29, data[28]));
			printf("Reason for kicking player %s : %s\n", target->name, reason);
			s_notify_kick_channel(ts_server, pl, target, reason, def_chan);
			move_player(pl, def_chan);
			/* TODO update player channel privileges etc... */

			free(reason);
		}
	}

	return NULL;
}

/**
 * Send a "player switched channel" notification to all players.
 *
 * @param s the server
 * @param pl the player who switched
 * @param from the channel the player was in
 * @param to the channel he is moving to
 */
void s_notify_switch_channel(struct server *s, struct player *pl, struct channel *from, struct channel *to)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 38;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0067bef0;		ptr += 4;	/* function code */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* ID of player who switched */
	*(uint32_t *)ptr = from->id;			ptr += 4;	/* ID of previous channel */
	*(uint32_t *)ptr = to->id;		ptr += 4;	/* channel the player switched to */
	*(uint16_t *)ptr = 1;			ptr += 2;	/* 1 = no pass, 0 = pass */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a request from a client to switch to another channel.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the sender
 * @param cli_len the size of cli_addr
 */
void *c_req_switch_channel(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	struct channel *to, *from;
	uint32_t to_id;
	char pass[30];
	uint32_t pub_id, priv_id;
	struct player *pl;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);
	memcpy(&to_id, data + 24, 4);
	to = get_channel_by_id(ts_server, to_id);
	bzero(pass, 30);
	strncpy(pass, data + 29, MIN(29, data[28]));

	if (pl != NULL && to != NULL) {
		send_acknowledge(pl);		/* ACK */
		if (!(to->flags & CHANNEL_FLAG_PASSWORD)
				|| strcmp(pass, to->password) == 0) {
			printf("Player switching to channel %s.\n", to->name);
			from = pl->in_chan;
			if (move_player(pl, to)) {
				s_notify_switch_channel(ts_server, pl, from, to);
				printf("Player moved, notify sent.\n");
				/* TODO change privileges */
			}
		}
	}
	return NULL;
}

/**
 * Notify players that a channel has been deleted
 *
 * @param s the server
 * @param del_id the id of the deleted channel
 */
void s_notify_channel_deleted(struct server *s, uint32_t del_id)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 30;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0073bef0;		ptr += 4;	/* function code */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = del_id;		ptr += 2;	/* ID of deleted channel */
	*(uint32_t *)ptr = 1;			ptr += 4;	/* ????? the previous 
								   ptr += 2 is not an error*/

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);

}

/**
 * Notify a player that his request to delete a channel failed (channel not empty)
 *
 * @param s the server
 * @param pl the player who wants to delete the channel
 * @param pkt_cnt the counter of the packet we failed to execute
 */
void s_resp_cannot_delete_channel(struct server *s, struct player *pl, uint32_t pkt_cnt)
{
	char *data, *ptr;
	int data_size = 30;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0xff93bef0;		ptr += 4;	/* function code */
	*(uint32_t *)ptr = pl->private_id;	ptr += 4;	/* private ID */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* public ID */
	*(uint32_t *)ptr = pl->f0_s_counter;	ptr += 4;	/* packet counter */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint16_t *)ptr = 0x00d1;		ptr += 2;	/* ID of player who switched */
	*(uint32_t *)ptr = pkt_cnt;		ptr += 4;	/* ??? */
	packet_add_crc_d(data, data_size);

	sendto(socket_desc, data, data_size, 0,
			(struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Handles a request by a client to delete a channel.
 * This request will fail if there still are people in the channel.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_adr
 */
void *c_req_delete_channel(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	struct channel *del;
	uint32_t pub_id, priv_id, pkt_cnt, del_id;
	struct player *pl;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	memcpy(&pkt_cnt, data + 12, 4);

	pl = get_player_by_ids(ts_server, pub_id, priv_id);
	memcpy(&del_id, data + 24, 4);
	del = get_channel_by_id(ts_server, del_id);

	if (pl != NULL) {
		send_acknowledge(pl);
		if (pl->global_flags & GLOBAL_FLAG_SERVERADMIN) {
			if (del == NULL || del->current_users > 0) {
				s_resp_cannot_delete_channel(ts_server, pl, pkt_cnt);
			} else if (destroy_channel_by_id(ts_server, del->id)) {
				s_notify_channel_deleted(ts_server, del_id);
			}
		}
	}
	return NULL;
}

/**
 * Send a notification to all players that target has been banned.
 *
 * @param s the server
 * @param pl the player who gave the order to ban
 * @param target the player who is banned
 * @param duration the duration of the ban (0 = unlimited)
 * @param reason the reason of the ban
 */
void s_notify_ban(struct server *s, struct player *pl, struct player *target, uint16_t duration, char *reason)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x0065bef0;		ptr += 4;	/* function code */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = target->public_id;	ptr += 4;	/* ID of player banned */
	*(uint16_t *)ptr = 2;			ptr += 2;	/* kick */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* banner ID */
	*(uint8_t *)ptr = strlen(reason);	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, strlen(reason));	ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			sendto(socket_desc, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a player ban request.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_ban(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id, ban_id;
	struct player *pl, *target;
	char *reason;
	uint16_t duration;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	memcpy(&ban_id, data + 24, 4);
	memcpy(&duration, data + 28, 2);

	pl = get_player_by_ids(ts_server, pub_id, priv_id);
	target = get_player_by_public_id(ts_server, ban_id);

	if (pl != NULL && target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(pl->global_flags & GLOBAL_FLAG_SERVERADMIN) {
			reason = strndup(data + 29, MIN(29, data[28]));
			add_ban(ts_server, new_ban(0, target->cli_addr->sin_addr, reason));
			printf("Reason for banning player %s : %s\n", target->name, reason);
			s_notify_ban(ts_server, pl, target, duration, reason);
			remove_player(ts_server, target);
			free(reason);
		}
	}
	return NULL;
}


/**
 * Send the list of bans to a player
 *
 * @param pl the player who asked for the list of bans
 */
void s_resp_bans(struct server *s, struct player *pl)
{
	char *data, *ptr;
	int data_size, tmp_size;
	struct ban *b;
	
	data_size = 24;
	data_size += 4;	/* number of bans */

	ar_each(struct ban *, b, s->bans)
		data_size += ban_to_data_size(b);
	ar_end_each;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint32_t *)ptr = 0x019bbef0;		ptr += 4;	/* function code */
	*(uint32_t *)ptr = pl->private_id;	ptr += 4;	/* private ID */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* public ID */
	*(uint32_t *)ptr = pl->f0_s_counter;	ptr += 4;	/* packet counter */
	/* packet version */			ptr += 4;	/* filled later */
	/* checksum */				ptr += 4;	/* filled at the end */
	*(uint16_t *)ptr = s->bans->used_slots;	ptr += 4;	/* number of bans */
	printf("number of bans : %i\n", s->bans->used_slots);
	ar_each(struct ban *, b, s->bans)
		tmp_size = ban_to_data(b, ptr);
		ptr += tmp_size;
	ar_end_each;
	
	packet_add_crc_d(data, data_size);
	printf("list of bans : sending %i bytes\n", data_size);
	sendto(socket_desc, data, data_size, 0,
			(struct sockaddr *)pl->cli_addr, pl->cli_len);

	pl->f0_s_counter++;

	free(data);
}

/**
 * Handle a player request for the list of bans*
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_list_bans(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id;
	struct player *pl;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);

	pl = get_player_by_ids(ts_server, pub_id, priv_id);

	if (pl != NULL) {
		send_acknowledge(pl);		/* ACK */
		s_resp_bans(ts_server, pl);
	}
	return NULL;
}

/**
 * Handles a request to remove a ban.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_remove_ban(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id;
	struct player *pl;
	struct in_addr ip;
	struct ban *b;

	memcpy(&priv_id, data + 4, 4);
	memcpy(&pub_id, data + 8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);

	if (pl != NULL && pl->global_flags & GLOBAL_FLAG_SERVERADMIN) {
		send_acknowledge(pl);		/* ACK */
		inet_aton(data+24, &ip);
		b = get_ban_by_ip(ts_server, ip);
		if (b != NULL)
			remove_ban(ts_server, b);
	}
	return NULL;
}
