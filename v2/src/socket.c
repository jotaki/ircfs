#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "socket.h"

int xconnect(const char * const server, const char * const port)
{
	struct addrinfo hints = { .ai_flags = 0 },
			* result = NULL,
			* rp = NULL;
	int err, sd = -1;
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	err = getaddrinfo(server, port, &hints, &result);
	if(err != 0)
		return -1;

	for(rp = result; rp != NULL; rp = rp->ai_next) {
		sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(0 > sd)
			continue;

		if(connect(sd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;

		close(sd);
		sd = -1;
	}

	if(result)
		freeaddrinfo(result);

	return sd;
}

void xclose(int sd)
{
	if(0 > sd) {
		shutdown(sd, SHUT_RDWR);
		close(sd);
	}
}

int skwrite(int sd, const void *buf, int count)
{
	int i = 0, r;

	while(i < count) {
		r = write(sd, ((void *)buf)+i, count-i);
		if(r == -1)
			return -1;

		i += r;
	}
	return i;
}

int skputs(const char * const s, int sd)
{
	return skwrite(sd, s, strlen(s));
}

int vskprintf(int sd, const char * const format, va_list ap)
{
	va_list copy;
	char *buf = NULL;
	int length = 0;

	va_copy(copy, ap);
	length = vsnprintf(buf, 0, format, copy);

	buf = malloc(length + 2);
	if(buf == NULL)
		return -1;

	memset(buf, 0, length + 2);
	va_copy(copy, ap);
	vsnprintf(buf, length+1, format, copy);

	if(skwrite(sd, buf, length) != length)
		length = -1;

	free(buf);
	buf = NULL;

	return length;
}

int skprintf(int sd, const char * const format, ...)
{
	va_list ap;
	int length = 0;

	va_start(ap, format);
	length = vskprintf(sd, format, ap);
	va_end(ap);

	return length;
}

int skread(int sd, void *buf, int count)
{
	return read(sd, buf, count);
}

int skgetc(int sd)
{
	int c = 0, r;

	r = skread(sd, (char*) &c, 1);
	c = (r <= 0? EOF: c);

	return c;
}

int skgets(char *buf, int max, int sd)
{
	int c, len = 0;

	while((c = skgetc(sd)) != '\n' && c != EOF && len < max) {
		if(c != '\r')
			buf[len++] = (char) c;
	}

	buf[len] = 0;
	return len;
}
