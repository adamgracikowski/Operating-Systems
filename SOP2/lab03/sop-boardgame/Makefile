CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address -pthread

.PHONY: all clean

all: sop-server sop-client

sop-client: sop-client.c boardgame-utils.h
	$(CC) $(CFLAGS) -o sop-client sop-client.c

sop-server: sop-server.c boardgame-utils.h
	$(CC) $(CFLAGS) -o sop-server sop-server.c
	
clean:
	rm -f sop-server sop-client
