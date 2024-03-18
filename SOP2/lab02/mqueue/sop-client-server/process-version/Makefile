CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address -pthread
LDFLAGS = -lrt

.PHONY: all clean

all: sop-server sop-client

sop-client: sop-client.c client-server-utils.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o sop-client sop-client.c

sop-server: sop-server.c client-server-utils.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o sop-server sop-server.c
	
clean:
	rm -f sop-server sop-client
