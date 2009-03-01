#ifndef __SERVER_PRIVILEGES_H__
#define __SERVER_PRIVILEGES_H__

#include "player.h"
#include "channel.h"

struct player;
struct channel;


#define PRIV_SERVER_ADMIN 0
#define PRIV_CHANNEL_ADMIN 1
#define PRIV_OPERATOR 2
#define PRIV_VOICE 3
#define PRIV_REGISTERED 4
#define PRIV_ANONYMOUS 5

#define SP_SIZE 88

/**
 * Define constants that correspond to the bit 
 * offset of the privilege
 * We keep 2 bytes empty at the begining, so 00..15 = nothing
 */
#define SP_ADM_DEL_SERVER		16
#define SP_ADM_ADD_SERVER		17
#define SP_ADM_LIST_SERVERS		18
#define SP_ADM_SET_PERMISSIONS		19
#define SP_ADM_CHANGE_USER_PASS		20
#define SP_ADM_CHANGE_OWN_PASS		21
#define SP_ADM_LIST_REGISTRATIONS	22
#define SP_ADM_REGISTER_PLAYER		23

#define SP_ADM_CHANGE_SERVER_CODECS	24
#define SP_ADM_CHANGE_SERVER_TYPE	25
#define SP_ADM_CHANGE_SERVER_PASS	26
#define SP_ADM_CHANGE_SERVER_WELCOME	27
#define SP_ADM_CHANGE_SERVER_MAXUSERS	28
#define SP_ADM_CHANGE_SERVER_NAME	29
#define SP_ADM_CHANGE_WEBPOST_URL	30
#define SP_ADM_CHANGE_SERVER_PORT	31

#define SP_ADM_START_SERVER		36
#define SP_ADM_STOP_SERVER		37
#define SP_ADM_MOVE_PLAYER		38
#define SP_ADM_BAN_IP			39

#define	SP_CHA_DELETE			40
#define SP_CHA_CREATE_MODERATED		41
#define SP_CHA_CREATE_SUBCHANNELED	42
#define SP_CHA_CREATE_DEFAULT		43
#define SP_CHA_CREATE_UNREGISTERED	44
#define SP_CHA_CREATE_REGISTERED	45
#define SP_CHA_JOIN_REGISTERED		46

#define SP_CHA_JOIN_WO_PASS		48
#define SP_CHA_CHANGE_CODEC		49
#define SP_CHA_CHANGE_MAXUSERS		50
#define SP_CHA_CHANGE_ORDER		51
#define SP_CHA_CHANGE_DESC		52
#define SP_CHA_CHANGE_TOPIC		53
#define SP_CHA_CHANGE_PASS		54
#define SP_CHA_CHANGE_NAME		55

#define SP_PL_GRANT_ALLOWREG		56
#define SP_PL_GRANT_VOICE		57
#define SP_PL_GRANT_AUTOVOICE		58
#define SP_PL_GRANT_OP			59
#define SP_PL_GRANT_AUTOOP		60
#define SP_PL_GRANT_CA			61
#define SP_PL_GRANT_SA			62

#define SP_PL_REGISTER_PLAYER		64
#define SP_PL_REVOKE_ALLOWREG		65
#define SP_PL_REVOKE_VOICE		66
#define SP_PL_REVOKE_AUTOVOICE		67
#define SP_PL_REVOKE_OP			68
#define SP_PL_REVOKE_AUTOOP		69
#define SP_PL_REVOKE_CA			70
#define SP_PL_REVOKE_SA			71

#define SP_PL_ALLOW_SELF_REG		78
#define SP_PL_DEL_REGISTRATION		79

#define SP_OTHER_CH_COMMANDER		80
#define SP_OTHER_CH_KICK		81
#define SP_OTHER_SV_KICK		82
#define SP_OTHER_TEXT_PL		83
#define SP_OTHER_TEXT_ALL_CH		84
#define SP_OTHER_TEXT_IN_CH		85
#define SP_OTHER_TEXT_ALL		86

struct server_privileges {
	char priv[6][SP_SIZE];
};

int sp_to_bitfield(struct server_privileges *sp, char *data);
struct server_privileges *new_sp_test(void);
struct server_privileges *new_sp(void);
int player_has_privilege(struct player *pl, int privilege, struct channel *ch);
void sp_print(struct server_privileges *sp);

#endif
