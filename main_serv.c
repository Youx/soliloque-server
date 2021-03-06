/*
 * soliloque-server, an open source implementation of the TeamSpeak protocol.
 * Copyright (C) 2009 Hugo Camboulive <hugo.camboulive AT gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <inttypes.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <signal.h>

#include "main_serv.h"
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
#include "config.h"
#include "log.h"
#include "queue.h"

#define MAX_MSG 1024

int nb_serv;
struct array *ss;

/* forced to be put as a global variable because
 * it's used in main AND a signal interrupt */
static int reload;

/* functions */
typedef void *(*packet_function)(char *data, unsigned int len, struct player *pl);
packet_function f0_callbacks[2][255];


static void init_callbacks(void)
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
	f0_callbacks[0][0xd2] = &c_req_change_chan_max_users;
	f0_callbacks[0][0xd4] = &c_req_change_chan_order;

	f0_callbacks[1][0x2c] = &c_req_leave;		/* client wants to leave */
	f0_callbacks[1][0x2d] = &c_req_kick_server;	/* client wants to kick someone from the server */
	f0_callbacks[1][0x2e] = &c_req_kick_channel;	/* client wants to kick someone from the channel */
	f0_callbacks[1][0x2f] = &c_req_switch_channel;	/* client wants to switch channels */
	f0_callbacks[1][0x30] = &c_req_change_player_attr; /* change player attributes */
	f0_callbacks[1][0x31] = &c_req_request_voice;	/* player requests voice */
	f0_callbacks[1][0x32] = &c_req_change_player_ch_priv;	/* change player channel privileges */
	f0_callbacks[1][0x33] = &c_req_change_player_sv_right;	/* change global flags */
	f0_callbacks[1][0x34] = &c_req_register_player;	/* create a new registration and associate the player to it */
	f0_callbacks[1][0x36] = &c_req_create_registration;	/* create a new registration */
	f0_callbacks[1][0x40] = &c_req_mute_player;	/* mute or unmute player */
	f0_callbacks[1][0x44] = &c_req_ip_ban;		/* client wants to ban an IP */
	f0_callbacks[1][0x45] = &c_req_ban;		/* client wants to ban someone */
	f0_callbacks[1][0x46] = &c_req_remove_ban;		/* client wants to unban someone */
	f0_callbacks[1][0x4a] = &c_req_move_player;	/* move a player from a chan to another */
	f0_callbacks[1][0x90] = &c_req_player_stats;	/* client wants connection stats for a player */
	f0_callbacks[1][0x95] = &c_req_server_stats;	/* client wants connection stats from the server */
	f0_callbacks[1][0x9a] = &c_req_list_bans;		/* client wants the list of bans */
	f0_callbacks[1][0xae] = &c_req_send_message;	/* client wants the list of bans */
	/* callbacks[0] = myfunc1; ... */
}

static void handle_connection_type_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	char *ptr = data + 2;
	uint16_t code = ru16(&ptr);

	logger(LOG_INFO, "Packet : Connection.");
	switch (code) {
	/* Client requesting a connection */
	case 3:
		handle_player_connect(data, len, cli_addr, cli_len, s);
		break;
	case 1:
		handle_player_keepalive(data, len, s);
		break;
	default:
		logger(LOG_WARN, "Unknown connection packet : 0xf4be%x.", ((uint16_t *)data)[1]);
	}
}

static packet_function get_f0_function(unsigned char * code)
{
	/* Function packets */
	if (code[3] == 0 || code[3] == 1) { /* 0 = server packet, 1 = client packet*/
		return f0_callbacks[code[3]][code[2]];
	}
	return NULL;
}



static void handle_control_type_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	packet_function func;
	uint8_t code[4] = {0,0,0,0};
	uint32_t public_id, private_id;
	struct player *pl;
	char *ptr;

	/* Valid code (no overflow) */
	memcpy(code, data, MIN(4, len));
	logger(LOG_INFO, "Packet : Control (0x%x).", *(uint32_t *)code);

	func = get_f0_function(code);
	if (func != NULL) {
		/* Check header size */
		if (len < 24) {
			logger(LOG_WARN, "Control packet too small to be valid.");
			return;
		}
		/* Check CRC */
		if (!packet_check_crc_d(data, len)) {
			logger(LOG_WARN, "Control packet (0x%x) has invalid CRC", *(uint32_t *)data);
			return;
		}
		/* Check if player exists */
		ptr = data + 4;
		private_id = ru32(&ptr);
		public_id = ru32(&ptr);
		pl = get_player_by_ids(s, public_id, private_id);
		/* Execute */
		if (pl != NULL) {
			pl->stats->activ_time = time(NULL);	/* update idle time */
			(*func)(data, len, pl);
		}
	} else {
		logger(LOG_WARN, "Function with code : 0x%"PRIx32" is invalid or is not implemented yet.", *(uint32_t *)code);
	}
}

static void handle_ack_type_packet(char *data, int len, struct sockaddr_in *cli_addr, struct server *s)
{
	struct player *pl;
	uint16_t sent_version, ack_version;
	uint32_t sent_counter, ack_counter;
	uint32_t public_id, private_id;
	char *sent, *ptr;

	logger(LOG_INFO, "Packet : ACK.");
	/* parse ACK packet */
	ptr = data + 2;
	ack_version = ru16(&ptr);
	private_id = ru32(&ptr);
	public_id = ru32(&ptr);
	ack_counter = ru32(&ptr);

	pl = get_player_by_ids(s, public_id, private_id);
	if (pl == NULL)
		pl = get_leaving_player_by_ids(s, public_id, private_id);
	if (pl != NULL) {
		pthread_mutex_lock(&pl->packets->mutex);

		sent = peek_at_queue(pl->packets);
		if (sent != NULL) {
			ptr = sent + 12;
			sent_counter = ru32(&ptr);
			sent_version = ru16(&ptr);

			if (sent_counter == ack_counter && ack_version <= sent_version)
				free(get_from_queue(pl->packets));
		}
		pthread_mutex_unlock(&pl->packets->mutex);
	}
}

static void handle_data_type_packet(char *data, int len, struct sockaddr_in *cli_addr, struct server *s)
{
	int res;
	logger(LOG_INFO, "Packet : Audio data.");
	res = audio_received(data, len, s);
	logger(LOG_INFO, "Return value : %i.", res);
}

/* Manage an incoming packet */
void handle_packet(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len, struct server *s)
{
	uint32_t pub, priv;
	struct player *pl;

	/* add some stats */
	sstat_add_packet(s->stats, len, 0);
	/* add some stats for the player if he exists */
	priv = GUINT32_FROM_LE(*(uint32_t *)(data + 4));
	pub = GUINT32_FROM_LE(*(uint32_t *)(data + 8));
	pl = get_player_by_ids(s, pub, priv);
	if (pl != NULL) {
		pl->stats->pkt_sent++;
		pl->stats->size_sent += len;
	}

	/* first a few tests */
	switch (GUINT16_FROM_LE(((uint16_t *)data)[0])) {
	case 0xbef0:		/* commands */
		handle_control_type_packet(data, len, cli_addr, cli_len, s);
		break;
	case 0xbef1:		/* acknowledge */
		handle_ack_type_packet(data, len, cli_addr, s);
		break;
	case 0xbef2: 		/* audio data */
		handle_data_type_packet(data, len, cli_addr, s);
		break;
	case 0xbef4:		/* connection and keepalives */
		handle_connection_type_packet(data, len, cli_addr, cli_len, s);
		break;
	default:
		logger(LOG_WARN, "Unvalid packet type field : 0x%x.", ((uint16_t *)data)[0]);
	}
}

static void print_help(char *progname)
{
	printf("%s\n", progname);
	printf("Usage : \n");
	printf(" -c <filename> filename of the config-file\n");
	printf(" -v show version\n");
	printf(" -h show this help\n");
}

static void print_version()
{
	printf("Soliloque Server version %s\n", VERSION);
}

/**
 * Makes the main function reach the end of the
 * pthread_joins cleanly. This is used by sigint
 * to exit cleanly, and sigusr1 to reload config
 */
void cleanup()
{
	size_t iter;
	struct server *s;
	struct config *cfg;

	ar_each(struct server *, s, iter, ss)
		cfg = s->conf;
		ar_remove(ss, s);
		server_stop(s);
	ar_end_each;

	/* cleanup database */
	dbi_conn_close(cfg->conn);
	dbi_shutdown();

	/* destroy config */
	destroy_config(cfg);
}

/**
 * signal function to exit cleanly
 */
void sigint()
{
	logger(LOG_INFO, "SIGINT received - clean exit");
	cleanup();
	signal(SIGINT, sigint);
}

/**
 * signal function to reload the config file
 */
void sigusr1()
{
	logger(LOG_INFO, "SIGUSR1 received - reloading configuration");
	reload = 1;
	cleanup();
	signal(SIGUSR1, sigusr1);
}


int main(int argc, char **argv)
{
	struct config *c;
	size_t iter;
	struct server *s;
	int i = 0;
	int val;
	int terminate = 0, wrongopt = 0, helpshown = 0;
	char *configfile = NULL;

	/* do some initialization of the finite state machine */
	init_callbacks();
	/* parse command line arguments */
	while ((val = getopt(argc, argv, "vhc:")) != -1) {
		switch (val) {
			case 'c':
				configfile = optarg;
				break;
			case 'h':
				print_help(argv[0]);
				terminate = 1;
				helpshown = 1;
				break;
			case 'v':
				print_version();
				terminate = 1;
				break;
			case '?':
				wrongopt = 1;
				terminate = 1;
				break;
		}
	}
	if (terminate != 0) {
		if (wrongopt == 1 && helpshown == 0)
			print_help(argv[0]);
		exit(1);
	}

	if (configfile == NULL)
		configfile = "sol-server.cfg";

	signal(SIGINT, sigint);
	signal(SIGUSR1, sigusr1);
	reload = 1; /* first launch, always load */
	while(reload) {
		/* default is only one launch then exit
		 * (except if sigusr1 is received) */
		reload = 0;
		c = config_parse(configfile);

		if (c == NULL) {
			logger(LOG_ERR, "Unable to read configuration file. Exiting.");
			exit(0);
		}
		set_config(c);

		init_db(c);
		if (!connect_db(c)) {
			logger(LOG_ERR, "Unable to connect to the database. Exiting.");
			exit(0);
		}
		ss = ar_new(2);
		db_create_servers(c, ss);

		ar_each(struct server *, s, iter, ss)
			db_create_channels(c, s);
			db_create_subchannels(c, s);
			db_create_registrations(c, s);
			db_create_sv_privileges(c, s);
			sp_print(s->privileges);
			db_create_pl_ch_privileges(c, s);
			logger(LOG_INFO, "Launching server %i", i);
			server_start(s);
			i++;
		ar_end_each;
		logger(LOG_INFO, "Servers initialized.");

		ar_each(struct server *, s, iter, ss)
			pthread_join(s->main_thread, NULL);
			pthread_join(s->packet_sender, NULL);
			free(s);
		ar_end_each;
		ar_free(ss);
	}
	logger(LOG_INFO, "All server threads ended. Exiting.");
	/* exit */
	return 0;
}
