CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address -pthread

.PHONY: all clean

all: sop-montecarlo

sop-bingo: sop-montecarlo.c
	$(CC) $(CFLAGS) $^ -o $@
	
clean:
	rm -f sop-montecarlo
