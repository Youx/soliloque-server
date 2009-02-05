#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "server.h"
#include <dbi/dbi.h>

struct config
{
	char *db_type;
	union {
		struct {
			char *path;
			char *db;
		} file;
		struct {
			char *host;
			int port;
			char *user;
			char *pass;
			char *db;
		} connection;
	} db;
	dbi_conn conn;
};

void destroy_config(struct config *c);
struct config *config_parse(char *filename);

#endif
