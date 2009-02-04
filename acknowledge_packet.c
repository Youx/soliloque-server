#include "acknowledge_packet.h"
#include "player.h"
#include "server_stat.h"

#include <assert.h>

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

	*(uint16_t *)ptr = PKT_TYPE_ACK;	ptr+=2;
	*(uint16_t *)ptr = 0x0000;		ptr+=2;
	*(uint32_t *)ptr = pl->private_id;	ptr+=4;
	*(uint32_t *)ptr = pl->public_id;	ptr+=4;
	*(uint32_t *)ptr = pl->f1_s_counter;	ptr+=4;

	assert (ptr - data == 16);

	send_to(pl->in_chan->in_server, data, 16, 0, (struct sockaddr *)pl->cli_addr, pl->cli_len);
	pl->f1_s_counter++;
}
