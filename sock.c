#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h>
#include <ctype.h>

#include "sock.h"

int skgetc(int *sd)
{
	int c = 0;
	int r = recv(*sd, (char *) &c, 1, 0);

	if(r == 0 || r == -1) {
		socket_debug("[%05d] EOF received.\n", *sd);
		c = EOF;
	}

	socket_info("[recv.%05d] 0x%02x '%c'\n", *sd, isprint(c) ? c : '0');
	return c;
}

int skgets(char *buf, int max, int *sd)
{
	int c, len = 0;

	while((c = skgetc(sd)) != '\n' && c != EOF && len < max) {
		if(c != '\r') {
			*buf++ = (char) c;
			++len;
		}
	}
	*buf = 0;

	socket_info("[read.%05d] size=%d, max=%d, \"%s\"\n", *sd,len,max,buf);
	return len;
}

int skputs(char *buf, int *sd)
{
	int i, k, size = strlen(buf);
	k = size;

	while(size > 0) {
		socket_info("[write.%05d] \"%s\"\n", *sd, buf);
		i = send(*sd, buf, size, 0);
		if(i == -1) {
			socket_debug("[%05d] Bad write: %s\n", *sd,
					strerror(errno));
			return i;
		}

		size -= i;
		buf += i;
	}
	return k;
}

int skwrite(int *sd, void *buf, int size)
{
	socket_info("[write.%05d] Sending %02d bytes\n", *sd, size);
	return send(*sd, buf, size, 0);
}

int skprintf(int *sd, char *fmt, ...)
{
	va_list ap;
	char buf[0x801];

	socket_info("[write.%05d] skprintf: WARNING: NOT THREAD SAFE!\n", *sd);

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	return skputs(buf, sd);
}

int *skbind(short port)
{
	int *sd = NULL;
	int yes = 1;
	struct sockaddr_in addr;

	sd = calloc(sizeof(int), 1);
	if(sd == NULL) {
		socket_debug("Out of Memory\n");
		return NULL;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if((*sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		socket_debug("Could not create socket: %s\n", strerror(errno));
		free(sd);
		return NULL;
	}

	if(setsockopt(*sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		socket_debug("[setsockopt.%05d] Failed: %s\n", *sd,
				strerror(errno));
		skclose(sd);
		return NULL;
	}

	if(bind(*sd, (struct sockaddr *) &addr,
				sizeof(struct sockaddr_in)) == -1) {
		socket_debug("[bind.%05d] Failed: %s\n", *sd,
				strerror(errno));
		skclose(sd);
		return NULL;
	}

	return sd;
}

int *skconnect(char *remote, short port)
{
	int *sd;
	struct sockaddr_in addr;
	struct hostent *host;

	sd = skbind(0);

	if(sd == NULL)
		return NULL;

	if((host = gethostbyname(remote)) == NULL) {
		free(sd);
		return NULL;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

	if(connect(*sd, (struct sockaddr *) &addr,
			sizeof(struct sockaddr_in)) == -1) {
		skclose(sd);
		sd = NULL;
	}

	return sd;
}

int *sklisten(short port, int backlog)
{
	int *sd = skbind(port);

	if(sd) {
		if(listen(*sd, backlog) < 0) {
			skclose(sd);
			sd = NULL;
		}
	}
	return sd;
}

int *skaccept(int *sd)
{
	int *c_sd = calloc(sizeof(int), 1);

	if(c_sd) {
		if((*c_sd = accept(*sd, NULL, NULL)) < 0) {
			free(c_sd);
			c_sd = NULL;
		}
	}
	return c_sd;
}

void skclose(int *sd)
{
	if(sd) {
		close(*sd);
		shutdown(*sd, 2);
		free(sd);
	}
}
