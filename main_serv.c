#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <resolv.h>
#include <poll.h>

#include "server.h"

#define MAX_MSG 1024

/* functions */
//void *(*callbacks[255])(char * data); 

/* context of the server */
struct server * ts_server;

static void test_init_server()
{
	/* Test server */
	ts_server = new_server();
	/* Add channels */
	add_channel(ts_server, new_predef_channel());
	add_channel(ts_server, new_predef_channel());
	add_channel(ts_server, new_predef_channel());
	destroy_channel_by_id(ts_server, 1);
	add_channel(ts_server, new_predef_channel());
	add_channel(ts_server, new_predef_channel());
	add_channel(ts_server, new_predef_channel());
	add_channel(ts_server, new_channel("Name", "Topic", "Desc", CHANNEL_FLAG_DEFAULT, CODEC_SPEEX_16_3, 0, 16));
	/* Add players */
	//new_default_player();
	add_player(ts_server, new_default_player());
	print_server(ts_server);
}

static void init_callbacks()
{
//f4be0300
	/* callbacks[0] = myfunc1; ... */
}




#define ERROR_IF(cond) \
if(cond) { \
	printf(strerror(errno)); \
	exit(1); \
}

#define WARNING_IF(cond) \
if(cond) { \
	printf(strerror(errno)); \
}

void handle_connection_type_packet(char * data, int len, struct sockaddr_in * cli_addr, unsigned int cli_len)
{
	printf("Connection packet!!!\n");
	switch( ((uint16_t *)data)[1] ) {
    /* Client requesting a connection */
    case 3:
      add_player(ts_server, new_player_from_data(data, len, cli_addr, cli_len));
      
    break;
  }
}


void handle_data_type_packet(char * data, int len)
{
		printf("Data packet!!!\n");
}

void handle_control_type_packet(char * data, int len)
{
		printf("Control packet!!!\n");
}

/* Manage an incoming packet */
void handle_packet(char * data, int len, struct sockaddr_in * cli_addr, unsigned int cli_len)
{
  printf("field 1 : 0x%x\n", ((uint16_t *)data)[0] );
	/* first a few tests */
	switch( ((uint16_t *)data)[0] ) {
		/* commands */
		case 0xf0be:
		break;
		/* acknowledge */
		case 0xf1be:
		break;
		/* audio data */
		case 0xf3be:
		break;
		/* connection and keepalives */
		case 0xbef4:
		handle_connection_type_packet(data, len, cli_addr, cli_len);
		break;
    default:
    printf("Unvalid packet type field\n");
	}
}


int main()
{
	int socket_desc, rc, n;
	unsigned int cli_len;
	struct sockaddr_in serv_addr, cli_addr;
	char data[MAX_MSG];
	struct pollfd socket_poll;
	int pollres;

	/* socket creation */
	socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
	ERROR_IF(socket_desc < 0);
	
	/* bind local server port */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(8767);
	rc = bind(socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	ERROR_IF(rc < 0);
	/* initialize for polling */
	socket_poll.fd = socket_desc;
	socket_poll.events = POLLIN;
	socket_poll.revents = 0;
	
	/* do some initialization of the finite state machine */
	init_callbacks();
	test_init_server();
	
	/* main loop */
	while(1) {
		/* infinite timeout, wait for data to be available */
		pollres = poll(&socket_poll, 1, -1);
		switch(pollres) {
			case 0:
				printf("Time limit expired\n");
				break;
			case -1:
				printf("Error occured while polling : %s\n", strerror(errno));
				break;
			default:
				printf("Packet received (pollres = %i)\n", pollres);
				n = recvfrom(socket_desc, data, MAX_MSG, 0,
				       (struct sockaddr *) &cli_addr, &cli_len);
				if(n == -1) {
					fprintf(stderr, "%s\n", strerror(errno));
				} else {
					printf("%i bytes received.\n", n);
					handle_packet(data, n, &cli_addr, cli_len);
				}
		}

	}
	/* exit */
	
	
}
