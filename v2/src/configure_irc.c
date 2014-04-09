#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ircfs.h"

int configure_irc(struct irc_settings_s ** irc_settings,
		const char * const connect_line)
{
	struct irc_settings_s * set;

	*irc_settings = NULL;

	set = calloc(1, sizeof(struct irc_settings_s));
	if(set == NULL)
		return ENOMEM;

	strncpy(set->buffer, connect_line, sizeof set->buffer);
	if(strlen(set->buffer) != strlen(connect_line)) {
		free(set);
		return E2BIG;
	}

	set->nickname = set->buffer;
	set->server   = strchr(set->buffer, '@');
	set->port     = strchr(set->buffer, ':');

	if(set->server == NULL) {
		free(set);
		return EINVAL;
	}
	*(set->server++) = '\0';

	if(set->port != NULL) {
		*(set->port++) = '\0';

		if(atoi(set->port) <= 0) {
			free(set);
			return EINVAL;
		}
	} else
		set->port = "6667";

	*irc_settings = set;
	return 0;
}

void free_irc_settings(struct irc_settings_s ** irc_settings)
{
	if(*irc_settings) {
		if((*irc_settings)->password)
			free((*irc_settings)->password);

		free(*irc_settings);
	}

	*irc_settings = NULL;
}
