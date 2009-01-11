#include "control_packet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <resolv.h>
#include <string.h>

#include "server.h"
#include "acknowledge_packet.h"

extern struct server *ts_server;

/**
 * Reply to a c_req_chans by sending packets containing
 * a data dump of the channels.
 *
 * @param pl the player we send the channel list to
 * @param s the server we will get the channels from
 *
 * TODO
 */
void s_resp_chans(struct player *pl, struct server *s)
{

}

/**
 * Handles a request for channels by the player.
 *
 * @param data the request packet
 * @param len the length of data
 * @param cli_addr the address of the client
 * @param cli_len the size of cli_addr
 */
void *c_req_chans(char *data, unsigned int len, struct sockaddr_in *cli_addr, unsigned int cli_len)
{
	uint32_t pub_id, priv_id;
	struct player *pl;

	memcpy(&priv_id, data+4, 4);
	memcpy(&pub_id, data+8, 4);
	pl = get_player_by_ids(ts_server, pub_id, priv_id);

	if(pl != NULL) {
		send_acknowledge(pl);
		s_resp_chans(pl, ts_server);
	}
	return NULL;
}
