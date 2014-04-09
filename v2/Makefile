CC= gcc
CFLAGS = -W -Wall -s -O2
INC = -Iinclude
LIB =
SRCS = $(wildcard ./src/*.c)
OBJS = $(SRCS:.c=.o)
BIN = ircfs

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

clean:
	rm -f $(BIN) $(OBJS)
