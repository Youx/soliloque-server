#include "server_privileges.h"

#include "player.h"

#include <stdlib.h>


/**
 * Convert a server privileges structure into a bitfield
 * (6 * 9 bytes)
 *
 * @param sp the server_privilege structure
 * @param data the buffer we are going to put the bitfield into
 *
 * @return the number of bytes written.
 */
int sp_to_bitfield(struct server_privileges *sp, char *data)
{
	int grp, priv, bit;

	for (grp = 0 ; grp < 6 ; grp++) {
		for (priv = 0 ; priv < SP_SIZE ; priv++) {
			bit = priv % 8;	/* 8 privileges / byte, we want the offset in the byte */
			data[(grp * 9) + (priv / 8)] |= (sp->privileges[grp][priv] << (7 - bit));
		}
	}
	return 6 * 9;
}

/**
 * Create an empty server_privileges structure
 *
 * @return the server privileges
 */
struct server_privileges *new_sp()
{
	return (struct server_privileges *)calloc(1, sizeof(struct server_privileges));
}

/**
 * Generate a test server privilege structure
 *
 * @return the structure
 */
struct server_privileges *new_sp_test()
{
	struct server_privileges *sp;
	sp = new_sp();

	/* administration */
	sp->privileges[PRIV_SERVER_ADMIN][SP_ADM_REGISTER_PLAYER] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_ADM_STOP_SERVER] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_ADM_START_SERVER] = 1;
	/* channel permissions */
	sp->privileges[PRIV_SERVER_ADMIN][SP_CHA_JOIN_WO_PASS] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_CHA_CHANGE_CODEC] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_CHA_CHANGE_MAXUSERS] = 1;
	/* player privileges */
	sp->privileges[PRIV_SERVER_ADMIN][SP_PL_GRANT_OP] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_PL_GRANT_AUTOOP] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_PL_GRANT_SA] = 1;
	/* other */
	sp->privileges[PRIV_SERVER_ADMIN][SP_OTHER_TEXT_ALL] = 1;
	sp->privileges[PRIV_SERVER_ADMIN][SP_OTHER_SV_KICK] = 1;

	return sp;
}

/**
 * Tell if a player belongs to a specified group.
 * NB : Everyone is in the "anonymous" group.
 *
 * @param pl the player
 * @param group the ID of a group (SA, CA, OP, Voice, Reg, All)
 *
 * @return 0 if he does not, something else if he does
 */
static int player_is_in_group(struct player *pl, int group)
{
	switch (group) {
	case PRIV_SERVER_ADMIN:
		return pl->global_flags & GLOBAL_FLAG_SERVERADMIN;
	case PRIV_CHANNEL_ADMIN:
		return pl->chan_privileges & CHANNEL_PRIV_CHANADMIN;
	case PRIV_OPERATOR:
		return pl->chan_privileges & CHANNEL_PRIV_OP;
	case PRIV_VOICE:
		return pl->chan_privileges & CHANNEL_PRIV_VOICE;
	case PRIV_REGISTERED:
		return pl->global_flags & GLOBAL_FLAG_REGISTERED;
	case PRIV_ANONYMOUS:	/* Everyone is this group */
		return 1;
	default:
		return 0;
	}
}

/**
 * Tell if a player has the specified privilege, by looking
 * in each group he belongs to if the permission is given to
 * that group.
 *
 * @param pl the player
 * @param privilege the privilege (as defined in server_privileges.h)
 *
 * @return 0 if he does not have the privilege, 1 if he does
 */
int player_has_privilege(struct player *pl, int privilege)
{
	int grp;

	for (grp = 0 ; grp < 6 ; grp++) {
		if (player_is_in_group(pl, grp)) {
			if (pl->in_chan->in_server->privileges->privileges[grp][privilege])
				return 1;
		}
	}
	return 0;
}

