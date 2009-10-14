#ifndef __PLAYER_CHANNEL_PRIVILEGE_H__
#define __PLAYER_CHANNEL_PRIVILEGE_H__

#include <stdint.h>

#define PL_CH_PRIV_UNREGISTERED 1
#define PL_CH_PRIV_REGISTERED 2

struct player_channel_privilege {
	int db_id;

	/* a privilege can be associated to either
	 * a player or a player's registration */
	char reg;
	union {
		struct player *pl;
		struct registration *reg;
	} pl_or_reg;
	struct channel *ch;

	int flags;
};

void destroy_player_channel_privilege(struct player_channel_privilege *priv);
struct player_channel_privilege *new_player_channel_privilege();
void player_clr_channel_privilege(struct player *pl, struct channel *ch, uint16_t bit);
void player_set_channel_privilege(struct player *pl, struct channel *ch, uint16_t bit);

#endif
