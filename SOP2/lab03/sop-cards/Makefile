CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -fsanitize=address -pthread

.PHONY: clean all

all: sop-cards

sop-client: sop-cards
	$(CC) $(CFLAGS) -o sop-cards sop-cards.c

clean:
	rm -f sop-cards
