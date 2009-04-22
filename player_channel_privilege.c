#include "player_channel_privilege.h"

#include "server.h"

void new_player_channel_privilege(struct channel *ch, struct player *pl)
{

}

void player_clr_channel_privilege(struct player *pl, struct channel *ch, uint16_t bit)
{
	struct player_channel_privilege *tmp_priv;

	tmp_priv = get_player_channel_privilege(pl, ch);
	tmp_priv->flags &= ~bit;
	/* update in the database if required */
	/*
	if (tmp_priv->reg == PL_CH_PRIV_REGISTERED)
		db_update_pl_chan_priv(tmp_priv);
	*/
}

void player_set_channel_privilege(struct player *pl, struct channel *ch, uint16_t bit)
{
	struct player_channel_privilege *tmp_priv;

	tmp_priv = get_player_channel_privilege(pl, ch);
	tmp_priv->flags |= bit;
	/* update in the database if required */
	/*
	if (tmp_priv->reg == PL_CH_PRIV_REGISTERED)
		db_update_pl_chan_priv(tmp_priv);
	*/
}

