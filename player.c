#include "player.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

struct player * new_player(char *nickname, char *login, char *machine)
{
	struct player * p;
	
	p = (struct player *)malloc(sizeof(struct player));
	
	
	return p;
}

struct player * new_default_player()
{
	return new_player("Nickname", "Login", "Test Machine 0.0");
}

struct player * new_player_from_data(char *data, int len, struct sockaddr_in * cli_addr, unsigned int cli_len)
{
	struct player * pl;
	char client[30];
	char machine[30];
	char nickname[30];
	char login[30];
	char password[30];
	
	char * ptr = data;
	
	/* Verify fields */
	
	/* Copy fields */
	ptr += 16;
	ptr += 4;
	strncpy(client, ptr+1, *ptr);
	client[(int)*ptr]=0;
	ptr += 30;
	strncpy(machine, ptr+1, *ptr);
	machine[(int)*ptr]=0;
	ptr += 30;
	/* not used yet */
	ptr += 10;
	
	strncpy(login, ptr+1, *ptr);
	login[(int)*ptr]=0;
	ptr += 30;
	strncpy(password, ptr+1, *ptr);
	password[(int)*ptr]=0;
	ptr += 30;
	strncpy(nickname, ptr+1, *ptr);
	nickname[(int)*ptr]=0;
	ptr += 30;
	
	printf("Client : %s\nMachine : %s\nLogin : %s\nPassword : %s\nNickname : %s\n", client, machine, login, password, nickname);
	/* Initialize player */
	pl = new_player(nickname, login, machine);
  pl->cli_addr = cli_addr;
  pl->cli_len = cli_len;
	return pl;
}
