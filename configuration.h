#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "server.h"

struct config
{
	char *db_type;
	union {
		char *path;
		struct {
			char *host;
			int port;
			char *user;
			char *pass;
			char *db;
		} connection;
	} db;
};

void destroy_config(struct config *c);
struct config *config_parse(char *filename);

#endif
