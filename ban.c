#include "ban.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

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
	struct ban *b = (struct ban *)calloc(1, sizeof(struct ban));

	if (b == NULL) {
		printf("(EE) new_ban, calloc failed : %s.\n", strerror(errno));
		return NULL;
	}
	
	b->duration = duration;
	b->ip = strdup(inet_ntoa(ip));
	b->reason = strdup(reason);
	if (b->ip == NULL || b->reason == NULL) {
		if (b->ip != NULL)
			free(b->ip);
		if (b->reason != NULL)
			free(b->reason);
		free(b);
		return NULL;
	}

	return b;
}

/**
 * Generate a testing ban
 *
 * @param x 0 or 1 to get different values
 *
 * @return the created ban
 */
struct ban *test_ban(int x)
{
	struct ban *b = (struct ban *)calloc(1, sizeof(struct ban));

	if (b == NULL) {
		printf("(EE) test_ban, calloc failed : %s.\n", strerror(errno));
		return NULL;
	}

	if (x == 0) {
		b->duration = 0;
		b->ip = "192.168.100.100";
		b->reason = "CBIENFAIT";
	} else {
		b->duration = 200;
		b->ip = "192.168.1.1";
		b->reason = "CBIENFAIT caca culotte blablablablabla";
	}
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
	/* IP (*) + Duration (2) + reason (*) */
	return (strlen(b->ip) +1) + 2 + (strlen(b->reason) +1);
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
	strcpy(ptr, b->ip);		ptr += strlen(b->ip) + 1;	/* ip */
	*(uint16_t *)ptr = b->duration;	ptr += 2;			/* duration in minutes */
	strcpy(ptr, b->reason);		ptr += strlen(b->reason) + 1;	/* reason */

	return ban_to_data_size(b);
}
