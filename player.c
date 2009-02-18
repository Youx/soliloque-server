#include "player.h"
#include "player_stat.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>


/**
 * Destroys a player and free its memory
 *
 * @param p the player we want to destroy
 */
void destroy_player(struct player *p)
{
	/* not always initialized */
	if (p->cli_addr)
		free(p->cli_addr);
	free(p);
}

/**
 * Allocate and initialize a new player.
 * 
 * @param nickname the nickname of the player
 * @param login the login of the player
 * @param machine a description of the machine of the player (ex : uname)
 *
 * @return the initialized player
 */
struct player *new_player(char *nickname, char *login, char *machine)
{
	struct player *p;
	
	p = (struct player *)calloc(1, sizeof(struct player));
	p->stats = new_plstat();
	strcpy(p->name, nickname);
	strcpy(p->machine, machine);
	strcpy(p->client, login);
	p->global_flags = GLOBAL_FLAG_SERVERADMIN | GLOBAL_FLAG_REGISTERED;	/* remove later */
	p->chan_privileges = CHANNEL_PRIV_CHANADMIN;
	p->f0_s_counter = 1;
	p->f0_c_counter = 1;
	p->f1_s_counter = 1;
	p->f1_c_counter = 1;
	p->f4_s_counter = 2; /* this is a mystery */
	p->f4_c_counter = 1;
	return p;
}

/**
 * Create a default player (for testing purposes)
 * 
 * @return the test player
 */
struct player *new_default_player()
{
	return new_player("Nickname", "Login", "Test Machine 0.0");
}

/**
 * Generate a player from a connection packet
 *
 * @param data the connection packet from the client
 * @param len the length of data
 * @param cli_addr the adress of the client
 * @param cli_len the length of cli_addr
 *
 * @return the allocated player
 */
struct player *new_player_from_data(char *data, int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	struct player *pl;
	char *client;
	char *machine;
	char *nickname;
	char *login;
	char *password;
	size_t tmp_size;
	char *ptr = data;
	uint16_t version[4];

	/* Verify fields */
	
	/* Copy fields */
	/* Bypass header */				ptr += 16;
	/* Bypass checksum */				ptr += 4;
	/* Copy the strings to a null terminated format */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of client */
	client = strndup(ptr, tmp_size);		ptr += 29;	/* client */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of machine */
	machine = strndup(ptr, tmp_size);		ptr += 29;	/* machine */
	version[0] = *(uint16_t *)ptr;			ptr += 2;
	version[1] = *(uint16_t *)ptr;			ptr += 2;
	version[2] = *(uint16_t *)ptr;			ptr += 2;
	version[3] = *(uint16_t *)ptr;			ptr += 2;
	/* not used yet */				ptr += 2;
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of login */
	login = strndup(ptr, tmp_size);			ptr += 29;	/* login */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of password */
	password = strndup(ptr, tmp_size);		ptr += 29;	/* password */
	tmp_size = MIN(29, *ptr);			ptr += 1;	/* size of nickname */
	nickname = strndup(ptr, tmp_size);		ptr += 29;	/* nickname */
	
	/* Initialize player */
	pl = new_player(nickname, login, machine);
	pl->version[0] = version[0];
	pl->version[1] = version[1];
	pl->version[2] = version[2];
	pl->version[3] = version[3];
	pl->stats->start_time = time(NULL);
	pl->stats->activ_time = time(NULL);
	/* Alloc adresses */
	pl->cli_addr = (struct sockaddr_in *)calloc(cli_len, sizeof(char));
	memcpy(pl->cli_addr, cli_addr, cli_len);
	pl->cli_len = cli_len;

	printf("machine : %s, login : %s, nickname : %s\n", pl->machine, pl->client, pl->name);
	free(client); free(machine); free(nickname); free(login); free(password);
	return pl;
}

/**
 * Converts a player to raw data to be transmitted
 *
 * @param pl the player to convert
 * @param data the data to write into (already allocated)
 *
 * @return the number of bytes written into data
 */
int player_to_data(struct player *pl, char *data)
{
	int size = player_to_data_size(pl);
	char *ptr = data;

	*(uint32_t *)ptr = pl->public_id;		ptr += 4;	/* public ID */
	*(uint32_t *)ptr = pl->in_chan->id;		ptr += 4;	/* channel ID */
	*(uint16_t *)ptr = pl->chan_privileges;		ptr += 2;	/* player chan privileges */
	*(uint16_t *)ptr = pl->global_flags;		ptr += 2;	/* player global flags */
	*(uint16_t *)ptr = pl->player_attributes;	ptr += 2;	/* player attributes */
	*ptr = MIN(29, strlen(pl->name));		ptr += 1;	/* player name size */
	strncpy(ptr, pl->name, *(ptr - 1));		ptr += 29;	/* player name */

	return size;
}

/**
 * Compute the number of bytes a player is going to take
 * once converted to raw data.
 *
 * @param pl the player
 *
 * @return the number of bytes that will be needed
 */
int player_to_data_size(struct player *pl)
{
	return 4 + 4 + 2 + 2 + 2 + 1 + 29; /* 44 */
}

void print_player(struct player *pl)
{
	printf("Player : %s\n", pl->name);
	printf("\tpublic ID  : 0x%x\n", pl->public_id);
	printf("\tprivate ID : 0x%x\n", pl->private_id);
	printf("\tmachine    : %s\n", pl->machine);
	printf("\tclient     : %s\n", pl->client);
}
