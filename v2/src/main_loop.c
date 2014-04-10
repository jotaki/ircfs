#include <stdio.h>
#include <sys/select.h>

#include "ircfs.h"
#include "queue.h"

void main_loop(int sd, int fd)
{
	fd_set fdrd;
	int r, hi;
	struct timeval timeout;

	hi = sd > fd? sd: fd;
	while(1) {
		timeout.tv_sec  = 2;
		timeout.tv_usec = 0;

		FD_ZERO(&fdrd);
		FD_SET(sd, &fdrd);
		FD_SET(fd, &fdrd);

		r = select(hi + 1, &fdrd, NULL, NULL, &timeout);
		if(r == -1) {
			perror("select(2) failed");
			continue;
		} else if(r == 0) {
			send_queue(sd);
			continue;
		}

		if(FD_ISSET(sd, &fdrd))
			process_irc(sd);
		else if(FD_ISSET(fd, &fdrd))
			if(process_file(fd, sd))
				break;
	}
}
