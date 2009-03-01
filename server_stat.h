#ifndef __SERVER_STAT_H__
#define __SERVER_STAT_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include "server.h"

struct server_stat
{
	/* array */
	size_t *pkt_sizes;
	struct timeval *pkt_timestamps;
	char *pkt_io;
	long pkt_max;

	/* bytes/packets received/sent */
	uint64_t pkt_sent;
	uint64_t pkt_rec;
	uint64_t size_sent;
	uint64_t size_rec;

	time_t start_time;

	uint64_t total_logins;
};


ssize_t send_to(struct server *s, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);
struct server_stat *new_sstat(void);
void sstat_add_packet(struct server_stat *st, size_t size, char in_out);
void compute_timed_stats(struct server_stat *st, uint32_t *res);
/*
 * void timersub(struct timeval *a, struct timeval *b,
                     struct timeval *res);
 */

#endif
