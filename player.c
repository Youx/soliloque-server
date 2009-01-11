#include "player.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

/**
 * Allocate and initialize a new player.
 * 
 * @param nickname the nickname of the player
 * @param login the login of the player
 * @param machine a description of the machine of the player (ex : uname)
 *
 * @return the initialized player
 */
struct player * new_player(char *nickname, char *login, char *machine)
{
	struct player * p;
	
	p = (struct player *)malloc(sizeof(struct player));
	
	
	return p;
}

/**
 * Create a default player (for testing purposes)
 * 
 * @return the test player
 */
struct player * new_default_player()
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
struct player * new_player_from_data(char *data, int len, struct sockaddr_in * cli_addr, unsigned int cli_len)
{
	struct player *pl;
	char *client;
	char *machine;
	char *nickname;
	char *login;
	char *password;
	size_t tmp_size;
	char *ptr = data;
	
	/* Verify fields */
	
	/* Copy fields */
	/* Bypass header */				ptr += 16;
	/* Bypass checksum */				ptr += 4;
	/* Copy the strings to a null terminated format */
	tmp_size = MIN(29, *ptr);			ptr++;		/* size of client */
	client = strndup(ptr, tmp_size);		ptr += 29;	/* client */
	tmp_size = MIN(29, *ptr);			ptr++;		/* size of machine */
	machine = strndup(ptr, tmp_size);		ptr += 29;	/* machine */
	/* not used yet */				ptr += 10;
	tmp_size = MIN(29, *ptr);			ptr++;		/* size of login */
	login = strndup(ptr, tmp_size);			ptr += 29;	/* login */
	tmp_size = MIN(29, *ptr);			ptr++;		/* size of password */
	password = strndup(ptr, tmp_size);		ptr += 29;	/* password */
	tmp_size = MIN(29, *ptr);			ptr++;		/* size of nickname */
	nickname = strndup(ptr, tmp_size);		ptr += 29;	/* nickname */
	
	/* Initialize player */
	pl = new_player(nickname, login, machine);
	pl->cli_addr = cli_addr;
	pl->cli_len = cli_len;

	free(client); free(machine); free(nickname); free(login); free(password);
	return pl;
}
