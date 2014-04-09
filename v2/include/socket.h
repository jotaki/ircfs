#ifndef IRCFS_SOCKET_H_
# define IRCFS_SOCKET_H_

# include <stdio.h>

int skwrite(int sd, const void *buf, int count);
int vskprintf(int sd,const char * const format, va_list ap);
int skprintf(int sd, const char * const format, ...);
int skputs(const char * const s, int sd);

int skread(int sd, void *buf, int count);
int skgetc(int sd);
int skgets(char *buf, int max, int sd);

int xconnect(const char * const server, const char * const port);
void xclose(int sd);

#endif	/* !IRCFS_SOCKET_H_ */
