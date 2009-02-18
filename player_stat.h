#ifndef __PLAYER_STAT_H__
#define __PLAYER_STAT_H__

struct player_stat
{
	uint16_t version[4];

	int connected_at; /* time of connection */

	int ping; /* test ping */
	int last_activity; /* time of last activity (=> idle)*/
	/* to determine the proportion of list packets */
	int packets;
	int packets_lost;
	/* ip adress */
	char *ip;
	
	int bytes_send;
	int bytes_received;
};

#endif
