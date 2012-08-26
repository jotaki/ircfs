#ifndef SOCK_H
# define SOCK_H

# ifdef DEBUG
#  define socket_debug(...) fprintf(stderr, __VA_ARGS__)
#  if DEBUG >= 1
#   define socket_info(...) fprintf(stderr, __VA_ARGS__)
#  else
#   define socket_info(...)
#  endif
# else
#  define socket_debug(...)
#  define socket_info(...)
# endif

/* SOCKET I/O */
int skgetc(int *sd);
int skgets(char *buf, int max, int *sd);
int skputs(char *buf, int *sd);
int skwrite(int *sd, void *buf, int size);
int skprintf(int *sd, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

/* SOCKET initalization */
int *skbind(short port);
int *skconnect(char *remote, short port);
int *sklisten(short port, int backlog);
int *skaccept(int *sd);

/* SOCKET cleanup */
void skclose(int *sd);

#endif
