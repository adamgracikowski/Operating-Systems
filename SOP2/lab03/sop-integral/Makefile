CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address -pthread

.PHONY: all clean

all: sop-integral

sop-integral: sop-integral.c
	$(CC) $(CFLAGS) -o sop-integral sop-integral.c
	
clean:
	rm -f sop-integral