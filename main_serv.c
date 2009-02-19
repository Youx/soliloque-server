#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <inttypes.h>
#include "server.h"
#include "connection_packet.h"
#include "control_packet.h"
#include "acknowledge_packet.h"
#include "audio_packet.h"
#include "packet_tools.h"
#include "server_stat.h"
#include "configuration.h"
#include "server_privileges.h"
#include "database.h"

#define MAX_MSG 1024

/* functions */
typedef void *(*packet_function)(char *data, unsigned int len, struct player *pl);
packet_function f0_callbacks[2][255];


void test_init_server(struct server *s)
{
	/* Add channels */
	add_channel(s, new_predef_channel());
	add_channel(s, new_predef_channel());
	add_channel(s, new_predef_channel());
	destroy_channel_by_id(s, 1);
	add_channel(s, new_predef_channel());
	add_channel(s, new_predef_channel());
	add_channel(s, new_predef_channel());
	add_channel(s, new_channel("Name", "Topic", "Desc", CHANNEL_FLAG_DEFAULT, CODEC_SPEEX_16_3, 0, 16));
	add_ban(s, test_ban(0));
	add_ban(s, test_ban(1));
	add_ban(s, test_ban(1));
	/* Add players */
	/*
	new_default_player();
	add_player(s, new_default_player());
	*/
	print_server(s);
}

static void init_callbacks()
{
	bzero(f0_callbacks[0], 255 * sizeof(packet_function));
	bzero(f0_callbacks[1], 255 * sizeof(packet_function));
	
	f0_callbacks[0][0x05] = &c_req_chans;		/* request chans and player list */
	f0_callbacks[0][0xc9] = &c_req_create_channel;	/* create a channel */
	f0_callbacks[0][0xcb] = &c_req_change_chan_pass;
	f0_callbacks[0][0xcd] = &c_req_change_chan_flag_codec;
	f0_callbacks[0][0xce] = &c_req_change_chan_name;
	f0_callbacks[0][0xcf] = &c_req_change_chan_topic;
	f0_callbacks[0][0xd0] = &c_req_change_chan_desc;
	f0_callbacks[0][0xd1] = &c_req_delete_channel;
	/*f0_callbacks[0][0xd2] = &c_req_change_chan_max_users;*/
	/*f0_callbacks[0][0xd4] = &c_req_change_chan_order;*/

	f0_callbacks[1][0x2c] = &c_req_leave;		/* client wants to leave */
	f0_callbacks[1][0x2d] = &c_req_kick_server;	/* client wants to kick someone from the server */
	f0_callbacks[1][0x2e] = &c_req_kick_channel;	/* client wants to kick someone from the channel */
	f0_callbacks[1][0x2f] = &c_req_switch_channel;	/* client wants to switch channels */
	f0_callbacks[1][0x30] = &c_req_change_player_attr; /* change player attributes */
	f0_callbacks[1][0x32] = &c_req_change_player_ch_priv;	/* change player channel privileges */
	f0_callbacks[1][0x33] = &c_req_change_player_sv_right;	/* change global flags */
	f0_callbacks[1][0x44] = &c_req_ip_ban;		/* client wants to ban an IP */
	f0_callbacks[1][0x45] = &c_req_ban;		/* client wants to ban someone */
	f0_callbacks[1][0x46] = &c_req_remove_ban;		/* client wants to unban someone */
	f0_callbacks[1][0x90] = &c_req_player_stats;	/* client wants connection stats for a player */
	f0_callbacks[1][0x95] = &c_req_server_stats;	/* client wants connection stats from the server */
	f0_callbacks[1][0x9a] = &c_req_list_bans;		/* client wants the list of bans */
	f0_callbacks[1][0xae] = &c_req_send_message;	/* client wants the list of bans */
	/* callbacks[0] = myfunc1; ... */
}

void handle_connection_type_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	printf("(II) Packet : Connection.\n");
	switch (((uint16_t *)data)[1]) {
	/* Client requesting a connection */
	case 3:
		handle_player_connect(data, len, cli_addr, cli_len, s);
		break;
	case 1:
		handle_player_keepalive(data, len, cli_addr, cli_len, s);
		break;
	default:
		printf("(WW) Unknown connection packet : 0xf4be%x.\n", ((uint16_t *)data)[1]);
	}
}

packet_function get_f0_function(unsigned char * code)
{
	/* Function packets */
	if (code[3] == 0 || code[3] == 1) { /* 0 = server packet, 1 = client packet*/
		return f0_callbacks[code[3]][code[2]];
	}
	return NULL;
}



void handle_control_type_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	packet_function func;
	uint8_t code[4] = {0,0,0,0};
	uint32_t public_id, private_id;
	struct player *pl;

	/* Valid code (no overflow) */
	memcpy(code, data, MIN(4, len));
	printf("(II) Packet : Control (0x%x).\n", *(uint32_t *)code);

	func = get_f0_function(code);
	if (func != NULL) {
		/* Check header size */
		if (len < 24) {
			printf("(WW) Control packet too small to be valid.\n");
			return;
		}
		/* Check CRC */
		/*if (packet_check_crc_d(data, len)) {
			printf("(WW) Control packet has invalid CRC\n");
			return;
		}*/
		/* Check if player exists */
		memcpy(&private_id, data + 4, 4);
		memcpy(&public_id, data + 8, 4);
		pl = get_player_by_ids(s, public_id, private_id);
		/* Execute */
		if (pl != NULL) {
			pl->stats->activ_time = time(NULL);	/* update idle time */
			(*func)(data, len, pl);
		}
	} else {
		printf("(WW) Function with code : 0x%"PRIx32" is invalid or is not implemented yet.\n", *(uint32_t *)code);
	}
}

static void handle_ack_type_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	printf("(II) Packet : ACK.\n");
}

static void handle_data_type_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	int res;
	printf("(II) Packet : Audio data.\n");
	res = audio_received(data, len, s);
	printf("(II) Return value : %i.\n", res);
}

/* Manage an incoming packet */
void handle_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	uint32_t pub, priv;
	struct player *pl;

	/* add some stats */
	sstat_add_packet(s->stats, len, 0);
	/* add some stats for the player if he exists */
	priv = *(uint32_t *)(data + 4);
	pub = *(uint32_t *)(data + 8);
	pl = get_player_by_ids(s, pub, priv);
	if (pl != NULL) {
		pl->stats->pkt_sent++;
		pl->stats->size_sent += len;
	}

	/* first a few tests */
	switch (((uint16_t *)data)[0]) {
	case 0xbef0:		/* commands */
		handle_control_type_packet(data, len, cli_addr, cli_len, s);
		break;
	case 0xbef1:		/* acknowledge */
		handle_ack_type_packet(data, len, cli_addr, cli_len, s);
		break;
	case 0xbef2: 		/* audio data */
		handle_data_type_packet(data, len, cli_addr, cli_len, s);
		break;
	case 0xbef4:		/* connection and keepalives */
		handle_connection_type_packet(data, len, cli_addr, cli_len, s);
		break;
	default:
		printf("(WW) Unvalid packet type field : 0x%x.\n", ((uint16_t *)data)[0]);
	}
}

int main()
{
	struct server **ss;
	struct config *c;
	int i;

	/* do some initialization of the finite state machine */
	init_callbacks();
	c = config_parse("sol-server.cfg");
	if (c == NULL) {
		printf("(EE) Unable to read configuration file. Exiting.\n");
		exit(0);
	}
	init_db(c);
	if (!connect_db(c)) {
		printf("(EE) Unable to connect to the database. Exiting.\n");
		exit(0);
	}
	ss = db_create_servers(c);

	for (i = 0 ; ss[i] != NULL ; i++) {
		db_create_channels(c, ss[i]);
		db_create_registrations(c, ss[i]);
		db_create_sv_privileges(c, ss[i]);
		sp_print(ss[i]->privileges);
		/* test_init_server(ss[i]); */
		printf("Launching server %i\n", i);
		server_start(ss[i]);
	}
	for (i = 0 ; ss[i] != NULL ; i++) {
		pthread_join(ss[i]->main_thread, NULL);
	}
	printf("Servers initialized.\n");
	/* exit */
	return 0;
}
