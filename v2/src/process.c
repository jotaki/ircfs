#include <stdlib.h>

#include "socket.h"
#include "irc.h"

static void pong_server(struct irc_s *irc);

struct irc_dispatch_s dispatcher =
{
	.on_numeric = NULL,
	.on_ping    = pong_server,
	.on_privmsg = NULL,
	.on_notice  = NULL,
	.on_join    = NULL,
	.on_part    = NULL,
	.on_quit    = NULL,
	.on_nick    = NULL,
};

static void pong_server(struct irc_s *irc)
{
	skprintf(*(int*)irc->data, "PONG %s\r\n", irc->message);
}

void process_irc(int sd)
{
	char *buf = NULL;
	struct irc_s *irc = NULL;

	printf("getting data.\n");

	buf = calloc(sizeof(char), 1025);
	if(buf == NULL) {
		perror("malloc(3)");
		return;
	}

	skgets(buf, 1024, sd);
	printf("%s\n", buf);

	irc = irc_parse(buf);
	if(irc != NULL) {
		irc->data = (void *) &sd;
		irc_dispatch(irc, &dispatcher);

		free(irc);
		irc = NULL;
	}
	
	free(buf);
	buf = NULL;
}

void process_file(int fd)
{
}

