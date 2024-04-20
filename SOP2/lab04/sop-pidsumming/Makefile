CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address

.PHONY: all clean

all: sop-server sop-client

sop-client: sop-client.c sop-socket.h
	$(CC) $(CFLAGS) -o sop-client sop-client.c

sop-server: sop-server.c sop-socket.h
	$(CC) $(CFLAGS) -o sop-server sop-server.c
	
clean:
	rm -f sop-server sop-client