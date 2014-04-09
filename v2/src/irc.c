#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "irc.h"

struct irc_s *irc_parse(const char * const buffer)
{
	struct irc_s *result = NULL;
	char *ptr = NULL,
	     *tmp = NULL;
	
	result = malloc(sizeof(struct irc_s));

	/* first verify we have allocated space */
	if(result == NULL)
		return result;

	/* now initialize the data structure */
	memset(result, 0, sizeof(struct irc_s));
	strncpy(result->buffer, buffer, IRC_BUFFER_SIZE);

	/* remove newline if necessary. */
	ptr = strrchr(result->buffer, '\n');
	if(ptr)
		*ptr = '\0';

	/* set command to input line, if it doesn't begin with ':' */
	ptr = result->buffer;
	if(*ptr != ':')
		result->command = ptr;
	else {
		/* command starts at first first space */
		result->command = strchr(ptr, ' ');
		if(result->command == NULL) { goto bad_result; }
		*(result->command++) = '\0';
		
		/* set src information */
		result->src.servername = 
		result->src.nickname = 
		result->source = &result->buffer[1];

		/* strip off nickname if possible */
		tmp = strchr(result->src.nickname, '!');
		if(tmp) {
			*(tmp++) = '\0';
			result->src.username = tmp;

			/* strip off hostname if possible  *
			 * XXX: If we got here, it should. */
			tmp = strchr(tmp, '@');
			if(tmp) {
				*(tmp++) = '\0';
				result->src.hostname = tmp;
			}
		}
	}

	/* strip off command */
	tmp = strchr(result->command, ' ');
	if(tmp == NULL) { goto bad_result; }
	*(tmp++) = '\0';

	/* set target/message */
	if(*tmp == ':') {
		*(tmp++) = '\0';
		result->message = tmp;
	} else {
		result->target = tmp;
		tmp = strchr(result->target, ' ');
		if(tmp) {
			*(tmp++) = '\0';
			/* skip leading ':' */
			if(*tmp == ':') { ++tmp; }
			result->message = tmp;
		}

		if(*result->target != '#')
			result->target = result->source;
	}

	/* return result */
	return result;

bad_result:
	/* cleanup code */
	free(result);
	result = NULL;

	return NULL;
}

static void safe_dispatch(struct irc_s * const irc, void (*func)())
{
	if(func)
		func(irc);
}

void irc_dispatch(struct irc_s * const irc,
		struct irc_dispatch_s * const dispatch)
{
	if(atoi(irc->command))
		safe_dispatch(irc, dispatch->on_numeric);
	else if(memcmp(irc->command, IRC_MSG_PING, SIRC_MSG_PING) == 0)
		safe_dispatch(irc, dispatch->on_ping);
	else if(memcmp(irc->command, IRC_MSG_PRIVMSG, SIRC_MSG_PRIVMSG) == 0)
		safe_dispatch(irc, dispatch->on_privmsg);
	else if(memcmp(irc->command, IRC_MSG_NOTICE, SIRC_MSG_NOTICE) == 0)
		safe_dispatch(irc, dispatch->on_notice);
	else if(memcmp(irc->command, IRC_MSG_JOIN, SIRC_MSG_JOIN) == 0)
		safe_dispatch(irc, dispatch->on_join);
	else if(memcmp(irc->command, IRC_MSG_PART, SIRC_MSG_JOIN) == 0)
		safe_dispatch(irc, dispatch->on_part);
	else if(memcmp(irc->command, IRC_MSG_QUIT, SIRC_MSG_QUIT) == 0)
		safe_dispatch(irc, dispatch->on_quit);
	else if(memcmp(irc->command, IRC_MSG_NICK, SIRC_MSG_NICK) == 0)
		safe_dispatch(irc, dispatch->on_nick);
}

#ifdef AS_APP
# include <stdio.h>
# define p(obj)		printf("%s = %s\n", #obj, obj)

int main(int argc, char *argv[])
{
	struct irc_s * irc;

	if(argc == 1)
		return 1;

	irc = irc_parse(argv[1]);
	if(irc) {
		p(irc->source);
		p(irc->target);
		p(irc->command);
		p(irc->message);
		p(irc->src.servername);
		p(irc->src.nickname);
		p(irc->src.username);
		p(irc->src.hostname);

		free(irc);
	}
	return 0;
}

#endif
