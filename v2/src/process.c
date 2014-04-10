#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>

#include "socket.h"
#include "irc.h"
#include "ircfs.h"

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
	//hmm? this is probably bad casting.
	skprintf((int)(unsigned long)irc->data, "PONG %s\r\n", irc->message);
}

void process_irc(int sd)
{
	char *buf = NULL;
	struct irc_s *irc = NULL;

	buf = calloc(sizeof(char), IRC_BUFFER_SIZE + 1);
	if(buf == NULL) {
		perror("malloc(3)");
		return;
	}

	skgets(buf, IRC_BUFFER_SIZE, sd);
	printf("%s\n", buf);

	irc = irc_parse(buf);
	if(irc != NULL) {
		irc->data = (void *)((unsigned long)sd);
		irc_dispatch(irc, &dispatcher);

		free(irc);
		irc = NULL;
	}
	
	free(buf);
	buf = NULL;
}

int process_file(int fd, int sd)
{
	char buf[BUFSIZ];
	struct inotify_event *i_event = (struct inotify_event *) buf;

	if(read(fd, buf, sizeof buf) < 0)
		return 0;

	if(i_event->len == 0)
		return 0;

	printf("Processing '%s'\n", i_event->name);

	if(i_event->name[0] == '#') {
		skprintf(sd, "JOIN %s\r\n", i_event->name);
		send_file(i_event->name);
	} else if(strcmp(i_event->name, "quit") == 0) {
		skprintf(sd, "QUIT :Leaving\r\n");
		return 1;
	}

	return 0;
}

