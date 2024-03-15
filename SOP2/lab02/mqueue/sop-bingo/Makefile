CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address -pthread
LDFLAGS = -lrt

.PHONY: all clean

all: sop-bingo

sop-bingo: sop-bingo.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
clean:
	rm -f sop-bingo
