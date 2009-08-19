#include "player_channel_privilege.h"

#include "server.h"
#include "database.h"
#include "log.h"

#include <stdlib.h>

struct player_channel_privilege *new_player_channel_privilege()
{
	struct player_channel_privilege *p;

	p = (struct player_channel_privilege *)calloc(1, sizeof(struct player_channel_privilege));
	if (p == NULL)
		logger(LOG_ERR, "new_player_channel_privilege: calloc failed!");
	return p;
}

void player_clr_channel_privilege(struct player *pl, struct channel *ch, uint16_t bit)
{
	struct player_channel_privilege *tmp_priv;

	tmp_priv = get_player_channel_privilege(pl, ch);
	tmp_priv->flags &= ~bit;
	/* update in the database if required */
	if (tmp_priv->reg == PL_CH_PRIV_REGISTERED)
		db_update_pl_chan_priv(ch->in_server->conf, tmp_priv);
}

void player_set_channel_privilege(struct player *pl, struct channel *ch, uint16_t bit)
{
	struct player_channel_privilege *tmp_priv;

	tmp_priv = get_player_channel_privilege(pl, ch);
	tmp_priv->flags |= bit;
	/* update in the database if required */
	if (tmp_priv->reg == PL_CH_PRIV_REGISTERED)
		db_update_pl_chan_priv(ch->in_server->conf, tmp_priv);
}

