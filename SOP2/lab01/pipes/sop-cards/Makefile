override CFLAGS=-Wall -Wextra -Wshadow -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror
endif

.PHONY: clean all

all: sop-cards

sop-lotto: sop-cards.c pipe-utils.h
	gcc $(CFLAGS) -o sop-cards sop-cards.c

clean:
	rm -f sop-cards
