#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "compat.h"
#include "channel.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <resolv.h>

/* Channel privileges */
#define CHANNEL_PRIV_CHANADMIN  1
#define CHANNEL_PRIV_OP   	2
#define CHANNEL_PRIV_VOICE  	4
#define CHANNEL_PRIV_AUTOOP 	8
#define CHANNEL_PRIV_AUTOVOICE  16

/* Global flags */
#define GLOBAL_FLAG_SERVERADMIN 1
#define GLOBAL_FLAG_ALLOWREG  	2
#define GLOBAL_FLAG_REGISTERED  4

/* Player attributes */
/* TODO: what are 1 and 2 ??? */
#define PL_ATTR_BLOCK_WHISPER	4
#define PL_ATTR_AWAY		8
#define PL_ATTR_MUTE_MIC	16
#define PL_ATTR_MUTE_SPK	32


struct player {
	int32_t public_id;
	int32_t private_id;
	
	char client[30];
	char machine[30];
	
	char name[30];
	
	uint16_t chan_privileges;
	uint16_t global_flags;
	uint16_t player_attributes;
	
	/* the channel the player is in */
	struct channel *in_chan;

	/* communication */
	struct sockaddr_in *cli_addr;
	unsigned int cli_len;

	/* packet counters */
	unsigned int f0_c_counter;
	unsigned int f0_s_counter;
	unsigned int f1_c_counter;
	unsigned int f1_s_counter;
	unsigned int f4_c_counter;
	unsigned int f4_s_counter;
};

void destroy_player(struct player *p);
struct player *new_player();
struct player *new_default_player();
struct player *new_player_from_data(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len);
int player_to_data(struct player *pl, char *data);
int player_to_data_size(struct player *pl);
void print_player(struct player *pl);
#endif
