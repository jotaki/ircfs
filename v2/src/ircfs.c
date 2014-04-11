#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "ircfs.h"
#include "socket.h"
#include "queue.h"

int main(int argc, char *argv[])
{
	char * mount_dir = NULL;
	struct irc_settings_s * settings = NULL;
	int sd, watcher, ircfd;

	if(argc == 1) {
		usage(argv[0]);
		return 1;
	}

	/*
	 * this function will exit if it fails.
	 */
	parse_opts(argc, argv, &settings, &mount_dir);
	if(mount_dir == NULL)
		mount_dir = "/tmp/ircfs";

	/*
	 * watch directory.
	 */
	if(watch_dir(mount_dir, &watcher, &ircfd)) {
		free_irc_settings(&settings);
		return 1;
	}

	/*
	 * setup signals
	 */
	signal(SIGINT, sig_quit_irc);
	signal(SIGTERM, sig_quit_irc);

	/*
	 * connect to server
	 */
	sd = xconnect(settings->server, settings->port);
	if(0 > sd) {
		printf("failed to connect to %s:%s\n", settings->server,
				settings->port);

		stop_watch_dir(mount_dir, &watcher, &ircfd);
		free_irc_settings(&settings);
		return 1;
	}

	/*
	 * assign mount dir to path
	 */
	set_full_path(mount_dir);

	/*
	 * Some basic IRC bootstrap
	 */

	if(settings->password) {
		//password would stay in skprintf()'s memory buffer.
		//skprintf(sd, "PASS %s\r\n", settings->password);
		skputs("PASS ", sd);
		skputs(settings->password, sd);
		skputs("\r\n", sd);
		memset(settings->password, 0, strlen(settings->password));

		free(settings->password);
		settings->password = NULL;
	}

	skprintf(sd, "NICK %s\r\n", settings->nickname);
	skprintf(sd, "USER %s 0 0 :%s\r\n", settings->nickname,
			settings->nickname);

	/*
	 * run the main loop for ircfs
	 */
	main_loop(sd, watcher);

	/*
	 * shutdown
	 */
	xclose(sd);
	stop_watch_dir(mount_dir, &watcher, &ircfd);
	free_irc_settings(&settings);

	return 0;
}

/*
 * maybe this is sneaky and stupid?
 */
void sig_quit_irc(int signo)
{
	FILE *fp = NULL;
	char *path = NULL;

	path = full_path("quit");
	if(path == NULL)
		exit(ENOMEM);

	fp = fopen(path, "w");
	if(fp) {
		fprintf(fp, "%d\n", signo);
		fclose(fp);
		fp = NULL;
	}

	free(path);
	path = NULL;
}
