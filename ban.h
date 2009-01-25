#ifndef __BAN_H__
#define __BAN_H__


#include <netinet/in.h>

struct ban
{
	uint16_t id;
	uint16_t duration;
	char *ip;
	char *reason;
};

struct ban *new_ban(uint16_t duration, struct in_addr ip, char *reason);

int ban_to_data_size(struct ban *b);
int ban_to_data(struct ban *b, char *dest);

#endif
