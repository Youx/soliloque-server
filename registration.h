#ifndef __REGISTRATION_H__
#define __REGISTRATION_H__

#include <openssl/sha.h>

struct registration
{
	char global_flags;	/* only serveradmin 0/1 */
	char name[30];
	char password[SHA256_DIGEST_LENGTH * 2 + 1];
};

struct registration *new_registration(void);

#endif
