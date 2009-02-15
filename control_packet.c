#include "control_packet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#include "server.h"
#include "channel.h"
#include "acknowledge_packet.h"
#include "array.h"
#include "packet_tools.h"
#include "server_stat.h"
#include "database.h"
#include "player.h"


/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the channels.
 *
 * @param pl the player we send the channel list to
 * @param s the server we will get the channels from
 *
 * TODO : split the channels over packets
 */
static void s_resp_chans(struct player *pl)
{
	char *data;
	int data_size = 0;
	struct channel *ch;
	char *ptr;
	int ch_size;
	struct server *s = pl->in_chan->in_server;

	/* compute the size of the packet */
	data_size += 24;	/* header */
	data_size += 4;		/* number of channels in packet */
	ar_each(struct channel *, ch, s->chans)
		data_size += channel_to_data_size(ch);
	ar_end_each;

	/* initialize the packet */
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;
	*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_LIST_CH;			ptr += 2;	/* */
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
	send_to(s, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Notify all players on the server that a new player arrived.
 *
 * @param pl the player who arrived
 * @param s the server
 */
void s_notify_new_player(struct player *pl)
{
	struct player *tmp_pl;
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;

	data_size = 24 + player_to_data_size(pl);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CREATE_PL;	ptr += 2;	/* */
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
			send_to(s, data, data_size, 0, (struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
		}
	ar_end_each;

	free(data);
}

/**
 * Send a "player disconnected" message to all players.
 *
 * @param p the player who left
 */
static void s_notify_player_left(struct player *p)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	struct server *s = p->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERLEFT;	ptr += 2;	/* */
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
			send_to(s, data, data_size, 0,
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
static void s_resp_players(struct player *pl)
{
	char *data;
	int data_size = 0;
	char *ptr;
	int p_size;
	int nb_players;
	struct player *pls[10];
	int i;
	int players_copied;
	struct server *s = pl->in_chan->in_server;

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
		*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
		*(uint16_t *)ptr = CTL_LIST_PL;			ptr += 2;	/* */
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
		send_to(s, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
		pl->f0_s_counter++;
		/* decrement the number of players to send */
		nb_players -= MIN(10, nb_players);
	}
	free(data);
}

static void s_resp_unknown(struct player *pl)
{
	char *data;
	char *ptr;
	int data_size = 283;
	struct server *s = pl->in_chan->in_server;

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

	send_to(s, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
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
void *c_req_chans(char *data, unsigned int len, struct player *pl)
{
	send_acknowledge(pl);		/* ACK */
	s_resp_chans(pl);	/* list of channels */
	usleep(250000);
	s_resp_players(pl);	/* list of players */
	usleep(250000);
	s_resp_unknown(pl);

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
void *c_req_leave(char *data, unsigned int len, struct player *pl)
{
	send_acknowledge(pl);		/* ACK */
	/* send a notification to all players */
	s_notify_player_left(pl);
	remove_player(pl->in_chan->in_server, pl);

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
static void s_notify_kick_server(struct player *kicker, struct player *kicked, char *reason)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	struct server *s = kicker->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERLEFT;	ptr += 2;	/* */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = kicked->public_id;	ptr += 4;	/* ID of player who left */
	*(uint16_t *)ptr = 2;			ptr += 2;	/* visible notification : kicked */
	*(uint32_t *)ptr = kicker->public_id;	ptr += 4;	/* kicker ID */
	*(uint8_t *)ptr = MIN(29, strlen(reason));	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, *(ptr - 1));	ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
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
void *c_req_kick_server(char *data, unsigned int len, struct player *pl)
{
	uint32_t target_id;
	char *reason;
	struct player *target;
	struct server *s = pl->in_chan->in_server;

	memcpy(&target_id, data + 24, 4);
	target = get_player_by_public_id(s, target_id);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(player_has_privilege(pl, SP_OTHER_SV_KICK, target->in_chan)) {
			reason = strndup(data + 29, MIN(29, data[28]));
			printf("Reason for kicking player %s : %s\n", target->name, reason);
			s_notify_kick_server(pl, target, reason);
			remove_player(s, pl);
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
 * @param kicked_to the channel the player is moved to
 */
static void s_notify_kick_channel(struct player *kicker, struct player *kicked, 
		char *reason, struct channel *kicked_to)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 68;
	struct server *s = kicker->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHKICK_PL;		ptr += 2;	/* */
	/* private ID */				ptr += 4;	/* filled later */
	/* public ID */					ptr += 4;	/* filled later */
	/* packet counter */				ptr += 4;	/* filled later */
	/* packet version */				ptr += 4;	/* not done yet */
	/* empty checksum */				ptr += 4;	/* filled later */
	*(uint32_t *)ptr = kicked->public_id;		ptr += 4;	/* ID of player who left */
	*(uint32_t *)ptr = kicker->public_id;		ptr += 4;	/* kicker ID */
	*(uint32_t *)ptr = kicked_to->id;		ptr += 4;	/* channel the player is moved to */
	*(uint16_t *)ptr = 0;				ptr += 2;	/* visible notification : kicked */
	*(uint8_t *)ptr = MIN(29,strlen(reason));	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, *(ptr - 1));		ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handles a channel kick request.
 */
void *c_req_kick_channel(char *data, unsigned int len, struct player *pl)
{
	uint32_t target_id;
	char *reason;
	struct player *target;
	struct channel *def_chan;
	struct server *s = pl->in_chan->in_server;

	memcpy(&target_id, data + 24, 4);
	target = get_player_by_public_id(s, target_id);
	def_chan = get_default_channel(s);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if (player_has_privilege(pl, SP_OTHER_CH_KICK, target->in_chan)) {
			reason = strndup(data + 29, MIN(29, data[28]));
			printf("Reason for kicking player %s : %s\n", target->name, reason);
			s_notify_kick_channel(pl, target, reason, def_chan);
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
static void s_notify_switch_channel(struct player *pl, struct channel *from, struct channel *to)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 38;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_SWITCHCHAN;	ptr += 2;	/* */
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
			send_to(s, data, data_size, 0,
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
void *c_req_switch_channel(char *data, unsigned int len, struct player *pl)
{
	struct channel *to, *from;
	uint32_t to_id;
	char pass[30];
	struct server *s = pl->in_chan->in_server;

	memcpy(&to_id, data + 24, 4);
	to = get_channel_by_id(s, to_id);
	bzero(pass, 30);
	strncpy(pass, data + 29, MIN(29, data[28]));

	if (to != NULL) {
		send_acknowledge(pl);		/* ACK */
		/* the player can join if one of these :
		 * - there is no password on the channel
		 * - he has the privilege to join without giving a password
		 * - he gives the correct password */
		if (!(to->flags & CHANNEL_FLAG_PASSWORD)
				|| player_has_privilege(pl, SP_CHA_JOIN_WO_PASS, to)
				|| strcmp(pass, to->password) == 0) {
			printf("Player switching to channel %s.\n", to->name);
			from = pl->in_chan;
			if (move_player(pl, to)) {
				s_notify_switch_channel(pl, from, to);
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
static void s_notify_channel_deleted(struct server *s, uint32_t del_id)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 30;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANDELETE;	ptr += 2;	/* */
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
			send_to(s, data, data_size, 0,
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
static void s_resp_cannot_delete_channel(struct player *pl, uint32_t pkt_cnt)
{
	char *data, *ptr;
	int data_size = 30;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANDELETE_ERROR;ptr += 2;	/* */
	*(uint32_t *)ptr = pl->private_id;	ptr += 4;	/* private ID */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* public ID */
	*(uint32_t *)ptr = pl->f0_s_counter;	ptr += 4;	/* packet counter */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint16_t *)ptr = 0x00d1;		ptr += 2;	/* ID of player who switched */
	*(uint32_t *)ptr = pkt_cnt;		ptr += 4;	/* ??? */
	packet_add_crc_d(data, data_size);

	send_to(s, data, data_size, 0,
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
void *c_req_delete_channel(char *data, unsigned int len, struct player *pl)
{
	struct channel *del;
	uint32_t pkt_cnt, del_id;
	struct server *s = pl->in_chan->in_server;

	memcpy(&pkt_cnt, data + 12, 4);

	memcpy(&del_id, data + 24, 4);
	del = get_channel_by_id(s, del_id);

	send_acknowledge(pl);
	if (player_has_privilege(pl, SP_CHA_DELETE, del)) {
		if (del == NULL || del->players->used_slots > 0) {
			s_resp_cannot_delete_channel(pl, pkt_cnt);
		} else {
			/* if the channel is registered, remove it from the db */
			printf("Flags : %i\n", del->flags);
			if ((del->flags & CHANNEL_FLAG_UNREGISTERED) == 0)
				db_unregister_channel(s->conf, del);
			s_notify_channel_deleted(s, del_id);
			destroy_channel_by_id(s, del->id);
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
static void s_notify_ban(struct player *pl, struct player *target, uint16_t duration, char *reason)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 64;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_PLAYERLEFT;	ptr += 2;	/* */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = target->public_id;	ptr += 4;	/* ID of player banned */
	*(uint16_t *)ptr = 2;			ptr += 2;	/* kick */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* banner ID */
	*(uint8_t *)ptr = MIN(29,strlen(reason));	ptr += 1;	/* length of reason message */
	strncpy(ptr, reason, *(ptr - 1));	ptr += 29;	/* reason message */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
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
void *c_req_ban(char *data, unsigned int len, struct player *pl)
{
	uint32_t ban_id;
	struct player *target;
	char *reason;
	uint16_t duration;
	struct server *s = pl->in_chan->in_server;

	memcpy(&ban_id, data + 24, 4);
	memcpy(&duration, data + 28, 2);

	target = get_player_by_public_id(s, ban_id);

	if (target != NULL) {
		send_acknowledge(pl);		/* ACK */
		if(player_has_privilege(pl, SP_ADM_BAN_IP, target->in_chan)) {
			reason = strndup(data + 29, MIN(29, data[28]));
			add_ban(s, new_ban(0, target->cli_addr->sin_addr, reason));
			printf("Reason for banning player %s : %s\n", target->name, reason);
			s_notify_ban(pl, target, duration, reason);
			remove_player(s, target);
			free(reason);
		}
	}
	return NULL;
}


/**
 * Send the list of bans to a player
 *
 * @param s the server
 * @param pl the player who asked for the list of bans
 */
static void s_resp_bans(struct player *pl)
{
	char *data, *ptr;
	int data_size, tmp_size;
	struct ban *b;
	struct server *s = pl->in_chan->in_server;
	
	data_size = 24;
	data_size += 4;	/* number of bans */

	ar_each(struct ban *, b, s->bans)
		data_size += ban_to_data_size(b);
	ar_end_each;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_BANLIST;		ptr += 2;	/* */
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
	send_to(s, data, data_size, 0,
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
void *c_req_list_bans(char *data, unsigned int len, struct player *pl)
{
	send_acknowledge(pl);		/* ACK */
	s_resp_bans(pl);
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
void *c_req_remove_ban(char *data, unsigned int len, struct player *pl)
{
	struct in_addr ip;
	struct ban *b;
	struct server *s = pl->in_chan->in_server;

	if(player_has_privilege(pl, SP_ADM_BAN_IP, NULL)) {
		send_acknowledge(pl);		/* ACK */
		inet_aton(data+24, &ip);
		b = get_ban_by_ip(s, ip);
		if (b != NULL)
			remove_ban(s, b);
	}
	return NULL;
}

/**
 * Handles a request to add an IP ban.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the sender
 * @param cli_len the length of cli_addr
 */
void *c_req_ip_ban(char *data, unsigned int len, struct player *pl)
{
	struct in_addr ip;
	uint16_t duration;
	struct server *s = pl->in_chan->in_server;

	if(player_has_privilege(pl, SP_ADM_BAN_IP, NULL)) {
		send_acknowledge(pl);		/* ACK */
		duration = *(uint16_t *)(data + 24);
		inet_aton(data+26, &ip);
		add_ban(s, new_ban(duration, ip, "IP BAN"));
	}
	return NULL;
}

/**
 * Send the server information to the player
 *
 * @param pl the player who requested the server info
 */
static void s_resp_server_stats(struct player *pl)
{
	int data_size = 100;
	char *data, *ptr;
	struct server *s = pl->in_chan->in_server;
	uint32_t stats[4] = {0, 0, 0, 0};
	
	compute_timed_stats(s->stats, stats);
	/* initialize the packet */
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;
	*(uint16_t *)ptr = PKT_TYPE_CTL;		ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_SERVSTATS;		ptr += 2;	/* */
	*(uint32_t *)ptr = pl->private_id;		ptr += 4;/* player private id */
	*(uint32_t *)ptr = pl->public_id;		ptr += 4;/* player public id */
	*(uint32_t *)ptr = pl->f0_s_counter;		ptr += 4;/* packet counter */
	/* packet version */				ptr += 4;
	/* empty checksum */				ptr += 4;
	*(uint64_t *)ptr = time(NULL) - s->stats->start_time;	ptr += 8;/* server uptime */
	*(uint16_t *)ptr = 2;				ptr += 2;/* server version */
	*(uint16_t *)ptr = 0;				ptr += 2;/* server version */
	*(uint16_t *)ptr = 20;				ptr += 2;/* server version */
	*(uint16_t *)ptr = 1;				ptr += 2;/* server version */
	*(uint32_t *)ptr = s->players->used_slots;	ptr += 4;/* number of players connected */
	*(uint64_t *)ptr = s->stats->pkt_sent;		ptr += 8;/* total bytes received */
	*(uint64_t *)ptr = s->stats->size_sent;		ptr += 8;/* total bytes sent */
	*(uint64_t *)ptr = s->stats->pkt_rec;		ptr += 8;/* total packets received */
	*(uint64_t *)ptr = s->stats->size_rec;		ptr += 8;/* total packets sent */
	*(uint32_t *)ptr = stats[0];			ptr += 4;/* bytes received/sec (last second) */
	*(uint32_t *)ptr = stats[1];			ptr += 4;/* bytes sent/sec (last second) */
	*(uint32_t *)ptr = stats[2]/60;			ptr += 4;/* bytes received/sec (last minute) */
	*(uint32_t *)ptr = stats[3]/60;			ptr += 4;/* bytes sent/sec (last minute) */
	*(uint64_t *)ptr = s->stats->total_logins;	ptr += 8;/* total logins */

	packet_add_crc_d(data, data_size);

	send_to(s, data, data_size, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f0_s_counter++;
	free(data);
}

/**
 * Handle a player request for server information
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_server_stats(char *data, unsigned int len, struct player *pl)
{
	send_acknowledge(pl);		/* ACK */
	s_resp_server_stats(pl);
	return NULL;
}

/**
 * Notify all players that a player's channel privilege
 * has been granted/revoked.
 *
 * @param pl the player who granted/revoked the privilege
 * @param tgt the player whose privileges are going to change
 * @param right the offset of the right (1 << right == CHANNEL_PRIV_XXX)
 * @param on_off switch this right on or off
 */
static void s_notify_player_ch_priv_changed(struct player *pl, struct player *tgt, char right, char on_off)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 34;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANGE_PL_CHPRIV;ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = tgt->public_id;	ptr += 4;/* ID of player whose channel priv changed */
	*(uint8_t *)ptr = on_off;		ptr += 1;/* switch the priv ON/OFF */
	*(uint8_t *)ptr = right;		ptr += 1;/* offset of the privilege (1<<right) */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;/* ID of the player who changed the priv */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a request to change a player's channel privileges
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_change_player_ch_priv(char *data, unsigned int len, struct player *pl)
{
	struct player *tgt;
	uint32_t tgt_id;
	char on_off, right;
	int priv_required;

	send_acknowledge(pl);		/* ACK */

	memcpy(&tgt_id, data + 24, 4);
	on_off = data[28];
	right = data[29];
	tgt = get_player_by_public_id(pl->in_chan->in_server, tgt_id);

	switch (1 << right) {
	case CHANNEL_PRIV_CHANADMIN:
		priv_required = (on_off == 0) ? SP_PL_GRANT_CA : SP_PL_REVOKE_CA;
		break;
	case CHANNEL_PRIV_OP:
		priv_required = (on_off == 0) ? SP_PL_GRANT_OP : SP_PL_REVOKE_OP;
		break;
	case CHANNEL_PRIV_VOICE:
		priv_required = (on_off == 0) ? SP_PL_GRANT_VOICE : SP_PL_REVOKE_VOICE;
		break;
	case CHANNEL_PRIV_AUTOOP:
		priv_required = (on_off == 0) ? SP_PL_GRANT_AUTOOP : SP_PL_REVOKE_AUTOOP;
		break;
	case CHANNEL_PRIV_AUTOVOICE:
		priv_required = (on_off == 0) ? SP_PL_GRANT_AUTOVOICE : SP_PL_REVOKE_AUTOVOICE;
		break;
	default:
		return NULL;
	}
	if (tgt != NULL && player_has_privilege(pl, priv_required, tgt->in_chan)) {
		printf("Player priv before : 0x%x\n", tgt->chan_privileges);
		if (on_off == 2)
			tgt->chan_privileges &= (0xFF ^ (1 << right));
		else if(on_off == 0)
			tgt->chan_privileges |= (1 << right);
		printf("Player priv after  : 0x%x\n", tgt->chan_privileges);
		s_notify_player_ch_priv_changed(pl, tgt, right, on_off);
	}
	return NULL;
}

/**
 * Notify all players that a player's global flags
 * has been granted/revoked.
 *
 * @param pl the player who granted/revoked the privilege
 * @param tgt the player whose privileges are going to change
 * @param right the offset of the right (1 << right == CHANNEL_PRIV_XXX)
 * @param on_off switch this right on or off
 */
static void s_notify_player_sv_right_changed(struct player *pl, struct player *tgt, char right, char on_off)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 34;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANGE_PL_SVPRIV;ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = tgt->public_id;	ptr += 4;/* ID of player whose global flags changed */
	*(uint8_t *)ptr = on_off;		ptr += 1;/* set or unset the flag */
	*(uint8_t *)ptr = right;		ptr += 1;/* offset of the flag (1 << right) */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;/* ID of player who changed the flag */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a request to change a player's global flags
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_change_player_sv_right(char *data, unsigned int len, struct player *pl)
{
	struct player *tgt;
	uint32_t tgt_id;
	char on_off, right;
	int priv_required;

	send_acknowledge(pl);		/* ACK */

	memcpy(&tgt_id, data + 24, 4);
	on_off = data[28];
	right = data[29];
	tgt = get_player_by_public_id(pl->in_chan->in_server, tgt_id);

	switch (1 << right) {
	case GLOBAL_FLAG_SERVERADMIN:
		priv_required = (on_off == 0) ? SP_PL_GRANT_SA : SP_PL_REVOKE_SA;
		break;
	case GLOBAL_FLAG_ALLOWREG:
		priv_required = (on_off == 0) ? SP_PL_GRANT_ALLOWREG : SP_PL_REVOKE_ALLOWREG;
		break;
	default:
		return NULL;
	}
	if (tgt != NULL && player_has_privilege(pl, priv_required, tgt->in_chan)) {
		printf("Player sv rights before : 0x%x\n", tgt->global_flags);
		if (on_off == 2)
			tgt->global_flags &= (0xFF ^ (1 << right));
		else if(on_off == 0)
			tgt->global_flags |= (1 << right);
		printf("Player sv rights after  : 0x%x\n", tgt->global_flags);
		s_notify_player_sv_right_changed(pl, tgt, right, on_off);
	}
	return NULL;
}

/**
 * Notify all players of a player's status change
 * has been granted/revoked.
 *
 * @param pl the player who granted/revoked the privilege
 * @param tgt the player whose privileges are going to change
 * @param right the offset of the right (1 << right == CHANNEL_PRIV_XXX)
 * @param on_off switch this right on or off
 */
static void s_notify_player_attr_changed(struct player *pl, uint16_t new_attr)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size = 30;
	struct server *s = pl->in_chan->in_server;

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANGE_PL_STATUS;ptr += 2;	/* */
	/* private ID */			ptr += 4;	/* filled later */
	/* public ID */				ptr += 4;	/* filled later */
	/* packet counter */			ptr += 4;	/* filled later */
	/* packet version */			ptr += 4;	/* not done yet */
	/* empty checksum */			ptr += 4;	/* filled later */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;	/* ID of player whose attr changed */
	*(uint16_t *)ptr = new_attr;		ptr += 2;	/* new attributes */

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a request to change a player's global flags
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player asking for it
 */
void *c_req_change_player_attr(char *data, unsigned int len, struct player *pl)
{
	uint16_t attributes;

	send_acknowledge(pl);		/* ACK */

	memcpy(&attributes, data + 24, 2);
	printf("Player sv rights before : 0x%x\n", pl->player_attributes);
	pl->player_attributes = attributes;
	printf("Player sv rights after  : 0x%x\n", pl->player_attributes);
	s_notify_player_attr_changed(pl, attributes);
	return NULL;
}

/**
 * Send a message to all players on the server
 *
 * @param pl the sender
 * @param color the hexadecimal color of the message
 * @param msg the message
 */
static void send_message_to_all(struct player *pl, uint32_t color, char *msg)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	/* header size (24) + color (4) + type (1) + name size (1) + name (29) + msg (?) */
	data_size = 24 + 4 + 1 + 1 + 29 + (strlen(msg) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_MESSAGE;		ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = color;		ptr += 4;/* color of the message */
	*(uint8_t *)ptr = 0;			ptr += 1;/* type of msg (0 = all) */
	*(uint8_t *)ptr = MIN(29, strlen(pl->name));	ptr += 1;/* length of the sender's name */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;/* sender's name */
	strcpy(ptr, msg);

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Send a message to all players in a channel
 *
 * @param pl the sender
 * @param ch the channel to send the message to
 * @param color the hexadecimal color of the message
 * @param msg the message
 */
static void send_message_to_channel(struct player *pl, struct channel *ch, uint32_t color, char *msg)
{
	char *data, *ptr;
	struct player *tmp_pl;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	/* header size (24) + color (4) + type (1) + name size (1) + name (29) + msg (?) */
	data_size = 24 + 4 + 1 + 1 + 29 + (strlen(msg) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_MESSAGE;		ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = color;		ptr += 4;/* color of the message */
	*(uint8_t *)ptr = 1;			ptr += 1;/* type of msg (1 = channel) */
	*(uint8_t *)ptr = MIN(29, strlen(pl->name));	ptr += 1;/* length of the sender's name */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;/* sender's name */
	strcpy(ptr, msg);

	ar_each(struct player *, tmp_pl, ch->players)
		*(uint32_t *)(data + 4) = tmp_pl->private_id;
		*(uint32_t *)(data + 8) = tmp_pl->public_id;
		*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
		packet_add_crc_d(data, data_size);
		send_to(s, data, data_size, 0,
				(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
		tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Send a message to a specific player
 *
 * @param pl the sender
 * @param tgt the receiver
 * @param color the hexadecimal color of the message
 * @param msg the message
 */
static void send_message_to_player(struct player *pl, struct player *tgt, uint32_t color, char *msg)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	/* header size (24) + color (4) + type (1) + name size (1) + name (29) + msg (?) */
	data_size = 24 + 4 + 1 + 1 + 29 + (strlen(msg) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_MESSAGE;		ptr += 2;	/* */
	*(uint32_t *)ptr = tgt->private_id;	ptr += 4;/* private ID */
	*(uint32_t *)ptr = tgt->public_id;	ptr += 4;/* public ID */
	*(uint32_t *)ptr = tgt->f0_s_counter;	ptr += 4;/* public ID */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = color;		ptr += 4;/* color of the message */
	*(uint8_t *)ptr = 2;			ptr += 1;/* type of msg (2 = private) */
	*(uint8_t *)ptr = MIN(29, strlen(pl->name));	ptr += 1;/* length of the sender's name */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;/* sender's name */
	strcpy(ptr, msg);

	packet_add_crc_d(data, data_size);
	send_to(s, data, data_size, 0,
			(struct sockaddr *)tgt->cli_addr, tgt->cli_len);
	tgt->f0_s_counter++;

	free(data);
}

/**
 * Handle a player request to send a message
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player sending the message
 */
void *c_req_send_message(char *data, unsigned int len, struct player *pl)
{
	uint32_t color;
	char msg_type;
	uint32_t dst_id;
	char *msg;
	struct channel *ch;
	struct player *tgt;
	
	send_acknowledge(pl);		/* ACK */

	memcpy(&color, data + 24, 4);
	memcpy(&msg_type, data + 28, 1);
	memcpy(&dst_id, data + 29, 4);
	msg = strdup(data + 33);

	switch (msg_type) {
	case 0:
		if (player_has_privilege(pl, SP_OTHER_TEXT_ALL, NULL))
			send_message_to_all(pl, color, msg);
		break;
	case 1:
		ch = get_channel_by_id(pl->in_chan->in_server, dst_id);
		/* If he is in the channel, check if he can send msg to his channel or any channel
		 * if he is not in the channel, check if he can send msg to any channel */
		if (ch != NULL) {
			if ((ch == pl->in_chan && player_has_privilege(pl, SP_OTHER_TEXT_IN_CH, ch))
					|| player_has_privilege(pl, SP_OTHER_TEXT_ALL_CH, ch))
			send_message_to_channel(pl, ch, color, msg);
		}
		break;
	case 2:
		tgt = get_player_by_public_id(pl->in_chan->in_server, dst_id);
		if (player_has_privilege(pl, SP_OTHER_TEXT_PL, tgt->in_chan))
				send_message_to_player(pl, tgt, color, msg);
		break;
	default:
		printf("Wrong type of message.\n");
	}
	free(msg);
	return NULL;
}

/**
 * Send a notification to all  players indicating that
 * a channel's name has changed
 *
 * @param pl the player who changed it
 * @param ch the channel changed
 * @param topic the new topic
 */
static void s_resp_chan_name_changed(struct player *pl, struct channel *ch, char *name)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + (strlen(name) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANGE_CH_NAME;	ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = ch->id;		ptr += 4;/* channel changed */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;/* player who changed */
	strcpy(ptr, name);

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a player request to change a channel's name
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the change
 */
void *c_req_change_chan_name(char *data, unsigned int len, struct player *pl)
{
	uint32_t ch_id;
	char *name;
	struct channel *ch;

	send_acknowledge(pl);
	memcpy(&ch_id, data + 24, 4);
	ch = get_channel_by_id(pl->in_chan->in_server, ch_id);

	if (ch != NULL) {
		if (player_has_privilege(pl, SP_CHA_CHANGE_NAME, ch)) {
			name = strdup(data + 28);
			free(ch->name);
			ch->name = name;
			s_resp_chan_name_changed(pl, ch, name);
		}
	}
	return NULL;
}

/**
 * Send a notification to all players indicating that
 * a channel's topic has changed
 *
 * @param pl the player who changed it
 * @param ch the channel changed
 * @param topic the new topic
 */
static void s_resp_chan_topic_changed(struct player *pl, struct channel *ch, char *topic)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + (strlen(topic) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANGE_CH_TOPIC;	ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = ch->id;		ptr += 4;/* channel changed */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;/* player who changed */
	strcpy(ptr, topic);

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a player request to change a channel's topic
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the change
 */
void *c_req_change_chan_topic(char *data, unsigned int len, struct player *pl)
{
	uint32_t ch_id;
	char *topic;
	struct channel *ch;

	send_acknowledge(pl);
	memcpy(&ch_id, data + 24, 4);
	ch = get_channel_by_id(pl->in_chan->in_server, ch_id);

	if (ch != NULL) {
		if (player_has_privilege(pl, SP_CHA_CHANGE_TOPIC, ch)) {
			topic = strdup(data + 28);
			free(ch->topic);
			ch->topic = topic;
			s_resp_chan_topic_changed(pl, ch, topic);
		}
	}
	return NULL;
}

/**
 * Send a notification to all  players indicating that
 * a channel's description has changed
 *
 * @param pl the player who changed it
 * @param ch the channel changed
 * @param topic the new topic
 */
static void s_resp_chan_desc_changed(struct player *pl, struct channel *ch, char *desc)
{
	char *data, *ptr;
	int data_size;
	struct server *s = pl->in_chan->in_server;
	struct player *tmp_pl;

	/* header size (24) + chan_id (4) + user_id (4) + name (?) */
	data_size = 24 + 4 + 4 + (strlen(desc) + 1);
	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;

	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CHANGE_CH_DESC;	ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = ch->id;		ptr += 4;/* channel changed */
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;/* player who changed */
	strcpy(ptr, desc);

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);

}

/**
 * Handle a player request to change a channel's description
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the change
 */
void *c_req_change_chan_desc(char *data, unsigned int len, struct player *pl)
{
	uint32_t ch_id;
	char *desc;
	struct channel *ch;

	send_acknowledge(pl);
	memcpy(&ch_id, data + 24, 4);
	ch = get_channel_by_id(pl->in_chan->in_server, ch_id);

	if (ch != NULL) {
		if (player_has_privilege(pl, SP_CHA_CHANGE_DESC, ch)) {
			desc = strdup(data + 28);
			free(ch->desc);
			ch->desc = desc;
			s_resp_chan_desc_changed(pl, ch, desc);
		}
	}
	return NULL;
}

/**
 * Notify all players on a server that a new channel has been created
 *
 * @param ch the new channel
 * @param creator the player who created the channel
 */
static void s_notify_channel_created(struct channel *ch, struct player *creator)
{
	char *data, *ptr;
	int data_size;
	struct player *tmp_pl;
	struct server *s = ch->in_server;

	data_size = 24 + 4;
	data_size += channel_to_data_size(ch);

	data = (char *)calloc(data_size, sizeof(char));
	ptr = data;


	*(uint16_t *)ptr = PKT_TYPE_CTL;	ptr += 2;	/* */
	*(uint16_t *)ptr = CTL_CREATE_CH;	ptr += 2;	/* */
	/* private ID */			ptr += 4;/* filled later */
	/* public ID */				ptr += 4;/* filled later */
	/* packet counter */			ptr += 4;/* filled later */
	/* packet version */			ptr += 4;/* not done yet */
	/* empty checksum */			ptr += 4;/* filled later */
	*(uint32_t *)ptr = creator->public_id;	ptr += 4;/* id of creator */
	channel_to_data(ch, ptr);

	ar_each(struct player *, tmp_pl, s->players)
			*(uint32_t *)(data + 4) = tmp_pl->private_id;
			*(uint32_t *)(data + 8) = tmp_pl->public_id;
			*(uint32_t *)(data + 12) = tmp_pl->f0_s_counter;
			packet_add_crc_d(data, data_size);
			send_to(s, data, data_size, 0,
					(struct sockaddr *)tmp_pl->cli_addr, tmp_pl->cli_len);
			tmp_pl->f0_s_counter++;
	ar_end_each;

	free(data);
}

/**
 * Handle a player request to create a new channel
 *
 * @param data the request packet
 * @param len the length of data
 * @param pl the player requesting the creation
 */
void *c_req_create_channel(char *data, unsigned int len, struct player *pl)
{
	struct channel *ch;
	size_t bytes_read;
	char *ptr;
	struct server *s;
	int priv_nok = 0;

	s = pl->in_chan->in_server;
	send_acknowledge(pl);

	ptr = data + 24;
	bytes_read = channel_from_data(ptr, len - (ptr - data), &ch);
	ptr += bytes_read;
	strncpy(ch->password, ptr, MIN(29, len - (ptr - data) - 1));
	/* Check the privileges */
	/* TODO : when we support subchannels, context will have to
	 * be changed to the parent channel (NULL if we create the
	 * channel at the root */
	if (ch->flags & CHANNEL_FLAG_UNREGISTERED) {
		if (!player_has_privilege(pl, SP_CHA_CREATE_UNREGISTERED, NULL))
			priv_nok++;
	} else {
		if (!player_has_privilege(pl, SP_CHA_CREATE_REGISTERED, NULL))
			priv_nok++;
	}
	if (ch->flags & CHANNEL_FLAG_DEFAULT
			&& !player_has_privilege(pl, SP_CHA_CREATE_DEFAULT, NULL))
		priv_nok++;
	if (ch->flags & CHANNEL_FLAG_MODERATED
			&& !player_has_privilege(pl, SP_CHA_CREATE_MODERATED, NULL))
		priv_nok++;
	if (ch->flags & CHANNEL_FLAG_SUBCHANNELS
			&& !player_has_privilege(pl, SP_CHA_CREATE_SUBCHANNELED, NULL))
		priv_nok++;

	if (priv_nok == 0) {
		add_channel(s, ch);
		printf("New channel created\n");
		print_channel(ch);
		if (! (ch->flags & CHANNEL_FLAG_UNREGISTERED))
			db_register_channel(s->conf, ch);
		print_channel(ch);
		s_notify_channel_created(ch, pl);
	}
	return NULL;
}
