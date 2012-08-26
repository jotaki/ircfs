CC= gcc
CFLAGS= -W -Wall -std=c99 -s -O2
SRCS=ircfs.c sock.c
OBJS=$(SRCS:.c=.o)

all: ircfs

ircfs: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm -rf $(OBJS) ircfs
