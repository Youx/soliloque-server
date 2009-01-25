#include "ban.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Create and initialize a new ban.
 * The ID is assigned only when the ban is added to the server.
 *
 * @param duration the duration of the ban (0 = unlimited)
 * @param ip the ip of the banned player
 * @param reason the reason/description of the ban
 *
 * @return the new ban
 */
struct ban *new_ban(uint16_t duration, struct in_addr ip, char *reason)
{
	struct ban *b = (struct ban *)calloc(sizeof(struct ban), 1);
	
	b->duration = duration;
	b->ip = strdup(inet_ntoa(ip));
	b->reason = strdup(reason);

	return b;
}

/**
 * Return the size a ban will take once converted to
 * raw data (to be transmitted).
 *
 * @param b the ban
 *
 * @return the number of bytes it will take
 */
int ban_to_data_size(struct ban *b)
{
	return 2 + 15 + 2 + strlen(b->reason) + 1;
}

/**
 * Convert a ban to raw data and put it into dest.
 *
 * @param b the ban
 * @param dest the destination buffer (already allocated)
 *
 * @return the number of bytes written
 */
int ban_to_data(struct ban *b, char *dest)
{
	char *ptr;

	ptr = dest;
	*(uint16_t *)ptr = b->id;	ptr += 2;	/* ID of ban */
	strncpy(ptr, b->ip, 15);	ptr += 15;	/* IP string */
	*(uint16_t *)ptr = b->duration;	ptr += 2;	/* duration of ban */
	strcpy(ptr, b->reason);		ptr += strlen(b->reason) + 1;

	return ban_to_data_size(b);
}
