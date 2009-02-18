#ifndef __PLAYER_STAT_H__
#define __PLAYER_STAT_H__

#include <stdint.h>

struct player_stat
{
	uint16_t version[4];

	int start_time; /* time of connection */

	int ping; /* test ping */
	int activ_time; /* time of last activity (=> idle)*/
	/* to determine the proportion of list packets */
	int pkt_rec;
	int pkt_sent;
	int size_rec;
	int size_sent;
	int pkt_lost;
	/* ip adress */
	char *ip;
	
	int bytes_send;
	int bytes_received;
};

struct player_stat *new_plstat();

#endif
