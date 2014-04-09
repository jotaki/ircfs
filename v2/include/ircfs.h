#ifndef IRCFS_H_
# define IRCFS_H_

/*
 * IRC settings
 */
struct irc_settings_s
{
	char buffer[512];

	char * nickname;
	char * server;
	char * port;
	char * password;
};

int configure_irc(struct irc_settings_s ** irc_settings,
		const char * const connect_line);
void free_irc_settings(struct irc_settings_s ** irc_settings);

/*
 * usage output
 */
void usage(const char * const app);

/*
 * parse command line options
 */
void parse_opts(int argc, char *argv[], struct irc_settings_s ** irc_settings,
		char ** mount_dir);

/*
 * watch directory (Uses inotify(7))
 * Note: requires Linux Kernel >= 2.6.27 and glibc >= 2.4
 */
int watch_dir(const char * const mount_dir, int * watcher, int * fd);
void stop_watch_dir(const char * const mount_dir, int * watcher, int * fd);

/*
 * process IRC socket and File creation
 */
void process_irc(int sd);
void process_file(int sd);

/*
 * main loop
 */
void main_loop(int sd, int fd);

#endif	/* !IRCFS_H_ */
