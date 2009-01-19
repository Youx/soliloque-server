#include "acknowledge_packet.h"
#include "player.h"
#include <assert.h>

extern struct server *ts_server;
extern int socket_desc;

/**
 * Send an acknowledge packet to a player
 * ACK packets consist of just a 16 bytes header
 *
 * @param pl the player
 */
void send_acknowledge(struct player *pl)
{
	char data[16];
	char *ptr = data;

	*(uint32_t *)ptr = 0x0000bef1;		ptr += 4;
	*(uint32_t *)ptr = pl->private_id;	ptr += 4;
	*(uint32_t *)ptr = pl->public_id;	ptr += 4;
	*(uint32_t *)ptr = pl->f1_s_counter;	ptr += 4;

	assert(ptr - data == 16);

	sendto(socket_desc, data, 16, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f1_s_counter++;
}
