/*
 * Copyright (c) 2012, Joseph Kinsella <jkinsella@spiryx.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <time.h>

#include "sock.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#ifndef DEFAULT_WAIT_TIME
# define DEFAULT_WAIT_TIME	20
#endif

#ifndef DEFAULT_QUEUE_BURST
# define DEFAULT_QUEUE_BURST	4
#endif

#ifndef MAX_CONNECT_LINE
# define MAX_CONNECT_LINE	300
#endif

#ifndef DEFAULT_IRC_PORT
# define DEFAULT_IRC_PORT	6667
#endif

#ifndef DEFAULT_LOG_LEVEL
# define DEFAULT_LOG_LEVEL	LOG_NOTICE
#endif

#ifndef LOG_LEVEL_FACILITY
# define LOG_LEVEL_FACILITY	LOG_USER
# define LOG_LEVEL_FACILITY_TEXT	"user"
#endif

#ifndef LOG_LEVEL_FACILITY_TEXT
# error "LOG_LEVEL_FACILITY_TEXT not set."
#endif

#ifndef LOG_LEVEL_PRIORITY
# define LOG_LEVEL_PRIORITY	LOG_INFO
# define LOG_LEVEL_PRIORITY_TEXT	"info"
#endif

#ifndef LOG_LEVEL_PRIORITY_TEXT
# error "LOG_LEVEL_PRIORITY_TEXT not set."
#endif

#ifdef DEBUG
# define debug_info(...)	fprintf(stderr, __VA_ARGS__)
#else
# define debug_info(...)
#endif

enum {
	/* return codes */
	SUCCESS = 0x00,
	FAILURE = -0x01,

	/* Running modes */
	RM_BACKGROUND = 0x00,
	RM_FOREGROUND = 0x01,

	/* Random constants */
	DATA_INITIALIZED = 0x01,	/* non-zero */
	USED_OPTION,
};

struct message_queue {
	time_t timestamp;
	char buffer[512];
	int size;
	struct message_queue *next, *end;
};

struct connection_info {
	unsigned char initialized;
	char *remote;
	char *nickname;
	unsigned short port;

	char buffer[MAX_CONNECT_LINE];
};

typedef struct {
	char *appname;
	char *pathname;
	unsigned char runmode;
	unsigned char dryrun;
	int log_level;
	int send_count;
	struct connection_info irc;
	struct message_queue *mq, *fq;
	int qburst;
	int wait_time;

	int *socket;
	int descriptor;
	int watch_descriptor;
	int *loop;
} app_config_t, *app_config_p;

struct ircbuffer {
	unsigned char is_ping, is_message;
	char *buffer, *fromline, *pong_host;
	char *nickname, *username, *hostname;
	char *recipient, *message;
};


extern int errno;
static int g_mainloop;

int qskprintf(app_config_p cfg, const char *format, ...);
int init_app_config(app_config_p cfg, int argc, char *argv[]);
void destroy_app_config(app_config_p cfg);
void usage(app_config_p cfg, int exit_code);
void dump_app_config(app_config_p cfg);
int parse_option(app_config_p cfg, char *option, char *extraopt);
int parse_connect_line(app_config_p cfg, const char *connect_line);
int validate_directory(app_config_p cfg, char *pathname);
int ircfs_printf(app_config_p cfg, int level, const char *format, ...);
int start_watcher(app_config_p cfg);
int start_socket(app_config_p cfg);
void main_loop(app_config_p cfg);
void process_socket(app_config_p cfg);
void process_descriptor(app_config_p cfg);
struct ircbuffer *parse_irc(char *buf, struct ircbuffer *ircbuf);
void send_queue(app_config_p cfg);
void update_queue(app_config_p cfg);
void abort_ircfs(int signo);
struct message_queue *q_add(app_config_p cfg, char *name, int size);
int str_replace(char *buffer, int chr, int rep);

void usage(app_config_p cfg, int exit_code)
{
	printf(
"Usage: %s [options] <connect line> <directory>                              \n"
"                                                                            \n"
"Where [options] can be one of:                                              \n"
"       -h              -- Show this output                                  \n"
"       -f              -- Run in foreground mode.                           \n"
"       -l <level>      -- Set log level (0:quiet, 7:noisy) (default: %d)    \n"
"       -b <burst>      -- Queue burst (default: %d)                         \n"
"       -w <seconds>    -- Wait <seconds> before reading file to channel.    \n"
"                          (The default value for this is: %d)               \n"
"       -D              -- Dry run, just print run time options and exit.    \n"
"                                                                            \n"
"<connect line> looks something like this: <nickname>@<server>[:port]        \n"
"       <nickname>      -- Defines the Nickname/Username/Realname to be used.\n"
"       <server>        -- The IRC host to connect to.                       \n"
"       <port>          -- The port to connect on. (default: %u)             \n"
"                                                                            \n"
"The supplied <directory> should not exist. It will be created by IrcFS.     \n"
"                                                                            \n"
"Options may be placed anywhere, the only order that matters is that the     \n"
"connect line must come before the directory. Example:                       \n"
"                                                                            \n"
"               %s nk@irc.server.net:6668 /tmp/server -f                     \n"
"               %s nk@irc.server.net:6668 -f /tmp/server                     \n"
"               %s -f nk@irc.server.net:6668 /tmp/server                     \n"
"               %s nk@irc.server.net ~/irc/server -l 2                       \n"
"                                                                            \n"
"                                                                            \n"
"", cfg->appname, DEFAULT_LOG_LEVEL, DEFAULT_QUEUE_BURST, DEFAULT_IRC_PORT,
	DEFAULT_WAIT_TIME, cfg->appname, cfg->appname, cfg->appname,
	cfg->appname);

	exit(exit_code);
}

int main(int argc, char *argv[])
{
	app_config_t app_config;
	pid_t child;

	g_mainloop = 1;
	if(init_app_config(&app_config, argc, argv) == FAILURE)
		usage(&app_config, EXIT_FAILURE);

	dump_app_config(&app_config);
	if(app_config.dryrun) {
		printf("Dry run! Exiting...\n");
		exit(EXIT_SUCCESS);
	}

	if(mkdir(app_config.pathname, 0755) < 0) {
		printf("Failed to create directory '%s': %s\n",
				app_config.pathname, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(app_config.runmode == RM_BACKGROUND) {
		child = fork();
		if(child == 0) {
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			setsid();

			openlog("ircfs", LOG_PID,
				LOG_LEVEL_FACILITY | LOG_LEVEL_PRIORITY);
		} else if(child < 0) {
			printf("Failed to fork: %s\n", strerror(errno));
			destroy_app_config(&app_config);
			exit(EXIT_FAILURE);
		} else
			exit(EXIT_SUCCESS);
	}

	if(start_watcher(&app_config) == FAILURE) {
		destroy_app_config(&app_config);
		exit(EXIT_FAILURE);
	}

	if(start_socket(&app_config) == FAILURE) {
		destroy_app_config(&app_config);
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, abort_ircfs);
	signal(SIGTERM, abort_ircfs);
	main_loop(&app_config);

	destroy_app_config(&app_config);
	exit(EXIT_SUCCESS);
	return SUCCESS;
}

void main_loop(app_config_p cfg)
{
	fd_set fdrd;
	int r, hi;
	struct timeval timeout;

	hi = cfg->descriptor > *cfg->socket ? cfg->descriptor :
		*cfg->socket;
	while(*cfg->loop) {
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		FD_ZERO(&fdrd);
		FD_SET(cfg->descriptor, &fdrd);
		FD_SET(*cfg->socket, &fdrd);

		r = select(hi + 1, &fdrd, NULL, NULL, &timeout);
		if(r == -1) {
			ircfs_printf(cfg, 3, "select(2) failed: %s\n",
					strerror(errno));
			continue;
		} else if(r == 0) {
			send_queue(cfg);
			update_queue(cfg);
			continue;
		}

		if(FD_ISSET(*cfg->socket, &fdrd))
			process_socket(cfg);
		else if(FD_ISSET(cfg->descriptor, &fdrd))
			process_descriptor(cfg);
	}
}

void destroy_app_config(app_config_p cfg)
{
	struct message_queue *q;

	if(remove(cfg->pathname) < 0)
		ircfs_printf(cfg, 0, "Failed to remove '%s': %s\n",
				cfg->pathname, strerror(errno));
	if(cfg->fq) {
		while(cfg->fq) {
			q = cfg->fq->next;
			free(cfg->fq);
			cfg->fq = q;
		}
	}

	if(cfg->mq) {
		while(cfg->mq) {
			q = cfg->mq->next;
			free(cfg->mq);
			cfg->mq = q;
		}
	}

	if(cfg->runmode == RM_BACKGROUND)
		closelog();

	if(cfg->watch_descriptor >= 0)
		inotify_rm_watch(cfg->descriptor, cfg->watch_descriptor);

	if(cfg->descriptor >= 0)
		close(cfg->descriptor);

	if(cfg->socket)
		skclose(cfg->socket);
}

int start_socket(app_config_p cfg)
{
	cfg->socket = skconnect(cfg->irc.remote, cfg->irc.port);
	if(cfg->socket == NULL) {
		ircfs_printf(cfg, 0, "Failed to connect to '%s:%u': %s\n",
				cfg->irc.remote, cfg->irc.port,
				strerror(errno));
		return FAILURE;
	}

	skprintf(cfg->socket, "NICK %s\r\n", cfg->irc.nickname);
	skprintf(cfg->socket, "USER %s * * :%s\r\n", cfg->irc.nickname,
			cfg->irc.nickname);

	return SUCCESS;
}

int start_watcher(app_config_p cfg)
{
	cfg->descriptor = inotify_init1(IN_CLOEXEC);
	if(cfg->descriptor < 0) {
		ircfs_printf(cfg, 0, "Failed to initialize inotify instance:"
				" %s\n", strerror(errno));
		return FAILURE;
	}

	cfg->watch_descriptor = inotify_add_watch(cfg->descriptor,
			cfg->pathname, IN_MODIFY);
	if(cfg->descriptor < 0) {
		ircfs_printf(cfg, 0, "Failed to watch '%s': %s\n",
				cfg->pathname, strerror(errno));
		return FAILURE;
	}

	return SUCCESS;
}

int init_app_config(app_config_p cfg, int argc, char *argv[])
{
	int rc = 0, argi = 0;

	/* Initialize memory to zero. */
	memset(cfg, 0, sizeof(app_config_t));
	cfg->descriptor = -1;	/* 0 could be a good value here. */
	cfg->watch_descriptor = -1;
	cfg->loop = &g_mainloop;
	cfg->wait_time = DEFAULT_WAIT_TIME;
	cfg->qburst = DEFAULT_QUEUE_BURST;

	/* Application name */
	cfg->appname = strrchr(argv[0], '/');
	if(cfg->appname)
		cfg->appname++;
	else
		cfg->appname = argv[0];

	cfg->log_level = DEFAULT_LOG_LEVEL;

	if(argc < 3)
		return FAILURE;

	/* process options */
	for(argi = 1; argi < argc; ++argi) {
		if(argv[argi][0] == '-') {
			rc = parse_option(cfg, &argv[argi][1], argi+1 >= argc ?
					NULL : argv[argi + 1]);
			if(rc == FAILURE)
				return FAILURE;
			else if(rc == USED_OPTION)
				argi += 1;

			continue;
		}

		if(cfg->irc.initialized == 0) {
			if(parse_connect_line(cfg, argv[argi]) == FAILURE)
				return FAILURE;
		} else if(cfg->pathname == NULL) {
			if(validate_directory(cfg, argv[argi]) == FAILURE)
				return FAILURE;
		} else {
			printf("Unknown argument: '%s'\n", argv[argi]);
			usage(cfg, EXIT_FAILURE);
		}
	}

	if(cfg->irc.initialized == 0 || cfg->pathname == NULL)
		return FAILURE;

	return SUCCESS;
}

int validate_directory(app_config_p cfg, char *pathname)
{
	struct stat dirinfo;

	if(stat(pathname, &dirinfo) == 0) {
		printf("The directory '%s' exists. This is not advised\n",
				pathname);
		return FAILURE;
	}

	if(errno != ENOENT) {
		debug_info("Failed to stat '%s': %s\n", pathname,
				strerror(errno));
		return FAILURE;
	}
	cfg->pathname = pathname;
	return SUCCESS;
}

int parse_connect_line(app_config_p cfg, const char *connect_line)
{
	int length = 0;
	char *temp = NULL; /* temporary value */

	length = strlen(connect_line);
	if(length == 0 || length > MAX_CONNECT_LINE - 1) {
		debug_info("Length of connection line is too long. String "
				"length given was '%d' but max is '%d'\n",
				length, MAX_CONNECT_LINE - 1);
		return FAILURE;
	}

	/* Buffer size checked previously, strncpy should not be necessary */
	strcpy(cfg->irc.buffer, connect_line);

	/* Set nickname */
	cfg->irc.nickname = cfg->irc.buffer;

	/* Set servername */
	cfg->irc.remote   = strchr(cfg->irc.buffer, '@');
	if(cfg->irc.remote == NULL) {
		debug_info("Failed to find '@' in connection line.\n");
		return FAILURE;
	}
	*(cfg->irc.remote++) = 0;

	/* Set port if necessary */
	temp = strchr(cfg->irc.remote, ':');
	if(temp != NULL) {
		*(temp++) = 0;
		cfg->irc.port = atoi(temp);
		if(cfg->irc.port <= 0) {
			debug_info("Invalid port: '%s'\n", temp);
			return FAILURE;
		}
	} else
		cfg->irc.port = DEFAULT_IRC_PORT;

	cfg->irc.initialized = DATA_INITIALIZED;

	return SUCCESS;
}

int parse_option(app_config_p cfg, char *option, char *extraopt)
{
	int rc = SUCCESS;

	switch(option[0]) {
		case 'D':
			cfg->dryrun = 1;
			break;
		case 'w':
			if(extraopt == NULL)
				usage(cfg, EXIT_FAILURE);
			cfg->wait_time = atoi(extraopt);
			rc = USED_OPTION;
			break;
		case 'b':
			if(extraopt == NULL)
				usage(cfg, EXIT_FAILURE);
			cfg->qburst = atoi(extraopt);
			rc = USED_OPTION;
			break;
		case 'f':
			cfg->runmode = RM_FOREGROUND;
			break;

		case 'h':
			usage(cfg, EXIT_SUCCESS);
			break;

		case 'l':
			if(extraopt == NULL)
				usage(cfg, EXIT_FAILURE);
			cfg->log_level = atoi(extraopt);
			rc = USED_OPTION;
			break;

		default:
			printf("Unknown option: %s\n", option);
			usage(cfg, EXIT_FAILURE);
	}

	return rc;
}

int ircfs_printf(app_config_p cfg, int level, const char *format, ...)
{
	va_list args;
	int length = 0;

	/* Drop messages above priority. (syslog style) */
	if(level > cfg->log_level)
		return 0;

	va_start(args, format);
	if(cfg->runmode == RM_FOREGROUND)
		length = vprintf(format, args);
	else
		vsyslog(LOG_LEVEL_FACILITY | LOG_LEVEL_PRIORITY, format, args);
	va_end(args);
		
	return length;
}

#define assert_return(x, y) 	if(!x) return y
struct ircbuffer *parse_irc(char *buf, struct ircbuffer *ircbuf)
{
	char *cmd, *params;

	memset(ircbuf, 0, sizeof(struct ircbuffer));
	assert_return(buf, ircbuf);

	ircbuf->buffer = strdup(buf);
	assert_return(ircbuf->buffer, ircbuf);

	cmd = buf = ircbuf->buffer;
	if(*buf == ':') {
		cmd = strchr(buf, ' ');
		assert_return(cmd, ircbuf);

		*(cmd++) = 0;
		ircbuf->fromline = buf;
	}

	params = strchr(cmd, ' ');
	assert_return(params, ircbuf);
	*(params++) = 0;
	assert_return(*params, ircbuf);
	
	if(strcasecmp(cmd, "ping") == 0) {
		ircbuf->is_ping = 1;	/* non-zero value */
		ircbuf->pong_host = params + 1;
	} else if(strcasecmp(cmd, "privmsg") == 0 ||
			strcasecmp(cmd, "notice") == 0) {
		ircbuf->is_message = 1;
		ircbuf->nickname = ircbuf->fromline + 1;
		ircbuf->username = strchr(ircbuf->nickname, '!');

		assert_return(ircbuf->username, ircbuf);
		*(ircbuf->username++) = 0;
		ircbuf->hostname = strchr(ircbuf->username, '@');
		assert_return(ircbuf->hostname, ircbuf);
		*(ircbuf->hostname++) = 0;

		ircbuf->recipient = params;
		ircbuf->message = strchr(ircbuf->recipient, ' ');
		assert_return(ircbuf->message, ircbuf);
		*(ircbuf->message++) = 0;
		assert_return(*ircbuf->message, ircbuf);
		++ircbuf->message;
	}

	return ircbuf;
}
#undef assert_return

void process_socket(app_config_p cfg)
{
	char buf[BUFSIZ];
	struct ircbuffer ircbuf;

	skgets(buf, sizeof(buf) - 1, cfg->socket);
	ircfs_printf(cfg, 7, "%s\n", buf);
	parse_irc(buf, &ircbuf);

	if(ircbuf.buffer == NULL)
		return;

	if(ircbuf.is_ping) {
		ircfs_printf(cfg, 7, "PONG! (%s)\n", ircbuf.pong_host);
		qskprintf(cfg, "PONG :%s\r\n", ircbuf.pong_host);
	} else if(ircbuf.is_message)
		ircfs_printf(cfg, 7, "<%s/%s> %s\n", ircbuf.nickname,
				ircbuf.recipient, ircbuf.message);

	free(ircbuf.buffer);
}

void process_descriptor(app_config_p cfg)
{
	char buf[BUFSIZ];
	struct inotify_event *i_event = (struct inotify_event *) buf;

	if(read(cfg->descriptor, buf, sizeof(buf)) < 0) {
		ircfs_printf(cfg, 3, "Read failed: %s\n", strerror(errno));
		return;
	}

	if(i_event->len == 0) {
		ircfs_printf(cfg, 6, "Received event (0x%04x) on unknown "
				"... skipping\n", i_event->mask);
		return;
	}

	ircfs_printf(cfg, 6, "Received event (0x%04x) on '%s'\n",
			i_event->mask, i_event->name);

	if(q_add(cfg, i_event->name, i_event->len))
		qskprintf(cfg, "JOIN %s\r\n", i_event->name);
		
	
}

void dump_app_config(app_config_p cfg)
{
	printf("Current configuration of IrcFS listed below:\n\n");

	printf("\t     Appname: %s\n", cfg->appname);
	printf("\t    Pathname: %s\n", cfg->pathname);
	printf("\tLog Facility: %s\n", LOG_LEVEL_FACILITY_TEXT);
	printf("\tLog Priority: %s\n", LOG_LEVEL_PRIORITY_TEXT);
	printf("\t   Log Level: %02d\n", cfg->log_level);
	printf("\t    IRC Host: %s\n", cfg->irc.remote);
	printf("\t    IRC Nick: %s\n", cfg->irc.nickname);
	printf("\t    IRC Port: %u\n", cfg->irc.port);
	printf("\t Queue Burst: %d\n", cfg->qburst);
	printf("\t   Wait time: %d\n", cfg->wait_time);
	printf("\tRunning Mode: %s\n", cfg->runmode == RM_FOREGROUND ?
			"Foreground" : "Background");

	printf("------------------------------------------------------\n\n");
}

void send_queue(app_config_p cfg)
{
	int i = 0, limit;
	struct message_queue *mesg;

	if(cfg->mq == NULL) {
		if(cfg->send_count > 0)
			--cfg->send_count;
		return;
	}

	limit = (cfg->qburst == 0 ? 10000 : cfg->qburst);
	while(i++ < limit && cfg->mq) {
		mesg = cfg->mq;
		ircfs_printf(cfg, 7, "Sending message '%s'\n", mesg->buffer);

		skwrite(cfg->socket, mesg->buffer, mesg->size);

		cfg->mq = mesg->next;
		if(cfg->mq)
			cfg->mq->end = mesg->end;
		free(mesg);
	}
	++cfg->send_count;
}
			
int qskprintf(app_config_p cfg, const char *format, ...)
{
	struct message_queue *mesg = NULL;
	va_list args;

	mesg = (struct message_queue *) calloc(1, sizeof(struct message_queue));
	if(mesg == NULL) return 0;

	va_start(args, format);
	mesg->size = vsnprintf(mesg->buffer, sizeof(mesg->buffer) - 1,
			format, args);
	va_end(args);

	if(cfg->mq == NULL)
		cfg->mq = mesg;
	else {
		if(cfg->mq->end) 
			cfg->mq->end->next = mesg;
		else
			cfg->mq->next = mesg;

		cfg->mq->end = mesg;
	}

	ircfs_printf(cfg, 7, "Accepted '%s' into msg queue\n", mesg->buffer);
	return mesg->size;
}

void abort_ircfs(int signo)
{
	signo = signo;	/* quiet gcc */
	g_mainloop = 0;
}

struct message_queue *q_add(app_config_p cfg, char *name, int size)
{
	struct message_queue *fileq;
       
	fileq = calloc(1, sizeof(struct message_queue));
	if(fileq == NULL) {
		ircfs_printf(cfg, 0, "out of memory? :( -> %s\n",
				strerror(errno));
		return NULL;
	}

	size = (size > (int) sizeof(fileq->buffer)) ?
			(int) sizeof(fileq->buffer) - 1 : size;
	strncpy(fileq->buffer, name, size);
	fileq->size = size;
	time(&fileq->timestamp);

	if(cfg->fq == NULL)
		cfg->fq = fileq;
	else {
		if(cfg->fq->end)
			cfg->fq->end->next = fileq;
		else
			cfg->fq->next = fileq;

		cfg->fq->end = fileq;
	}

	return fileq;
}

/*
 * TODO: Make better
 */
void update_queue(app_config_p cfg)
{
	struct message_queue *fileq;
	time_t now;
	char buffer[300];
	char filename[512];
	FILE *fp;

	while(cfg->fq) {
		fileq = cfg->fq;
		time(&now);
		if(now < (fileq->timestamp + cfg->wait_time))
			break;

		snprintf(filename, sizeof(filename), "%s/%s", cfg->pathname,
				fileq->buffer);

		ircfs_printf(cfg, 7, "Processing file '%s'\n", filename);

		fp = fopen(filename, "r");
		if(fp == NULL) {
			ircfs_printf(cfg, 0, "Could not open '%s': %s\n",
					filename, strerror(errno));
			break;
		}

		while(fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
			str_replace(buffer, '\r', 0);
			str_replace(buffer, '\n', 0);
			/* should probably test against isprint() */
			qskprintf(cfg, "PRIVMSG %s :%s\r\n", fileq->buffer,
					buffer);
		}
		qskprintf(cfg, "PART %s\r\n", fileq->buffer);
		fclose(fp);

		if(remove(filename) < 0) 
			ircfs_printf(cfg, 0, "Failed to remove '%s': %s\n",
					filename, strerror(errno));

		cfg->fq = fileq->next;
		if(cfg->fq)
			cfg->fq->end = fileq->end;

		free(fileq);
	}
}

int str_replace(char *buffer, int chr, int rep)
{
	int count = 0;

	if(buffer)
		while(*buffer) {
			if(*buffer == chr) {
				*buffer = rep;
				++count;
			}
			++buffer;
		}

	return count;
}
