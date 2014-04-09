#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "ircfs.h"

#define PASS_LENGTH	32

extern char * optarg;
extern int optind;

static char * server_pass()
{
	struct termios orig, temp;
	char * password, * ptr;

	password = calloc(sizeof(char), PASS_LENGTH);
	if(password == NULL)
		return NULL;

	/* ensure echo is off. XXX: handle potential errors? */
	tcgetattr(STDIN_FILENO, &orig);
	temp = orig;
	temp.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &temp);

	while(strlen(password) < 2) {
		printf("Password: ");
		fflush(stdout);

		ptr = fgets(password, PASS_LENGTH, stdin);
		printf("\n");
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &orig);

	ptr = strchr(password, '\n');
	if(ptr)
		*ptr = 0;

	ptr = strchr(password, '\r');
	if(ptr)
		*ptr = 0;

	return password;
}


void parse_opts(int argc, char *argv[], struct irc_settings_s **irc_settings,
		char ** mount_dir)
{
	int opt, has_pass = 0;

	while((opt = getopt(argc, argv, "hd:p")) > 0) {
		switch(opt) {
		case 'h':
			usage(argv[0]);
			exit(0);
			break;

		case 'p':
			++has_pass;
			break;

		case 'd':
			*mount_dir = optarg;
			break;

		default:
			usage(argv[0]);
			exit(1);
		}
	}

	if(optind >= argc) {
		usage(argv[0]);
		exit(1);
	}

	opt = configure_irc(irc_settings, argv[optind]);
	if(opt > 0) {
		printf("Error parsing connect line: %s\n", strerror(opt));
		exit(opt);
	}

	if(has_pass)
		(*irc_settings)->password = server_pass();
}
