#include <libconfig.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"

static struct server *parse_server(config_setting_t *set)
{
	struct server *s;
	config_setting_t *cpl;
	s = new_server();

	/* get name */
	cpl = config_setting_get_member(set, "name");
	strncpy(s->server_name, config_setting_get_string(cpl), 29);
	printf("Name : %s\n", s->server_name);
	/* get password */
	cpl = config_setting_get_member(set, "pass");
	strncpy(s->password, config_setting_get_string(cpl), 29);
	printf("Pass : %s\n", s->password);
	/* get port */
	cpl = config_setting_get_member(set, "port");
	s->port = config_setting_get_int(cpl);
	printf("Port : %i\n", s->port);
	/* get welcome message */
	cpl = config_setting_get_member(set, "welcome");
	strncpy(s->welcome_msg, config_setting_get_string(cpl), 255);
	printf("Welcome msg : %s\n", s->welcome_msg);

	return s;
}
struct server **parse()
{
	config_t cfg;
	config_setting_t *servers;
	config_setting_t *serv;
	struct server **ss;
	int i = 0;

	config_init(&cfg);
	if (config_read_file(&cfg, "sol-server.cfg") == CONFIG_FALSE) {
		printf("Error loading file\n");
	}

	servers = config_lookup(&cfg, "servers");
	if (servers == NULL) {
		printf("Error loading servers.\n");
	}

	while ((serv = config_setting_get_elem(servers, i)) != NULL)
		i++;
	ss = (struct server **)calloc(sizeof(struct server *), i + 1);

	i = 0;
	while ((serv = config_setting_get_elem(servers, i)) != NULL) {
		printf("i = %i\n", i);	
		ss[i] = parse_server(serv);
		i++;
	}
	return ss;
}

