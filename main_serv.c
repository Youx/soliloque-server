#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <resolv.h>
#include <poll.h>
#include <inttypes.h>
#include "server.h"
#include "connection_packet.h"
#include "control_packet.h"
#include "acknowledge_packet.h"
#include "audio_packet.h"
#include "packet_tools.h"
#include "server_stat.h"

#define MAX_MSG 1024

/* functions */
typedef void *(*packet_function)(char *data, unsigned int len, struct player *pl);
packet_function f0_callbacks[255];


static struct server *test_init_server()
{
	/* Test server */
	struct server *s = new_server();
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
	return s;
}

static void init_callbacks()
{
	bzero(f0_callbacks, 255 * sizeof(packet_function));
	f0_callbacks[0x2c] = &c_req_leave;		/* client wants to leave */
	f0_callbacks[0x2d] = &c_req_kick_server;	/* client wants to kick someone from the server */
	f0_callbacks[0x2e] = &c_req_kick_channel;	/* client wants to kick someone from the channel */
	f0_callbacks[0x2f] = &c_req_switch_channel;	/* client wants to switch channels */
	f0_callbacks[0x30] = &c_req_change_player_attr; /* change player attributes */
	f0_callbacks[0x32] = &c_req_change_player_ch_priv;	/* change player channel privileges */
	f0_callbacks[0x33] = &c_req_change_player_sv_right;	/* change global flags */
	f0_callbacks[0x44] = &c_req_ip_ban;		/* client wants to ban an IP */
	f0_callbacks[0x45] = &c_req_ban;		/* client wants to ban someone */
	f0_callbacks[0x46] = &c_req_remove_ban;		/* client wants to unban someone */
	f0_callbacks[0x95] = &c_req_server_stats;	/* client wants connection stats from the server */
	f0_callbacks[0x9a] = &c_req_list_bans;		/* client wants the list of bans */
	f0_callbacks[0xae] = &c_req_send_message;	/* client wants the list of bans */
	//f4be0300
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
	if (code[3] == 0) { /* 0 = server packet, 1 = client packet*/
		switch (code[2]) {
		case 0x05:
			return &c_req_chans;
		case 0xc9:
		case 0xd1:	/* delete channel */
			return &c_req_delete_channel;
		default:
			return NULL;
		}
	} else {
		if (f0_callbacks[code[2]])
			return f0_callbacks[code[2]];
		else
			return NULL;
	}
}


#define ERROR_IF(cond) \
	if(cond) { \
		printf("(EE) %s", strerror(errno)); \
		exit(1); \
	}

#define WARNING_IF(cond) \
	if(cond) { \
		printf("(WW) %s", strerror(errno)); \
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
static void handle_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	/* add some stats */
	sstat_add_packet(s->stats, len, 0);
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
	int rc, n;
	unsigned int cli_len;
	struct sockaddr_in serv_addr, cli_addr;
	char data[MAX_MSG];
	struct pollfd socket_poll;
	int pollres;
	struct server *ts_server;

	/* do some initialization of the finite state machine */
	init_callbacks();
	ts_server = test_init_server();

	/* socket creation */
	ts_server->socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
	ERROR_IF(ts_server->socket_desc < 0);
	
	/* bind local server port */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(8767);
	rc = bind(ts_server->socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	ERROR_IF(rc < 0);
	/* initialize for polling */
	socket_poll.fd = ts_server->socket_desc;
	socket_poll.events = POLLIN;
	socket_poll.revents = 0;


	/* main loop */
	while (1) {
		/* infinite timeout, wait for data to be available */
		pollres = poll(&socket_poll, 1, -1);
		switch(pollres) {
		case 0:
			printf("(EE) Time limit expired\n");
			break;
		case -1:
			printf("(EE) Error occured while polling : %s\n", strerror(errno));
			break;
		default:
			cli_len = sizeof(cli_addr);
			n = recvfrom(ts_server->socket_desc, data, MAX_MSG, 0,
					(struct sockaddr *) &cli_addr, &cli_len);
			if (n == -1) {
				fprintf(stderr, "(EE) %s\n", strerror(errno));
			} else {
				printf("(II) %i bytes received.\n", n);
				handle_packet(data, n, &cli_addr, cli_len, ts_server);
			}
		}

	}
	/* exit */
}
