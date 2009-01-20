#include "packet_sender.h"
#include "server.h"

void *packet_sender_thread(void *args)
{
	struct server *s;
	int socket_desc;
	struct player *p;
	size_t iter;

	s = ((struct server **)args)[0];
	socket_desc = ((int *)args)[1];
	p = ((struct player **)args)[2];

	while(1) {
		/* Can we */
		sem_wait(&s->send_packets);
		ar_each(struct player *, p, iter, s->players)
			
		ar_end_each;
	}
}
