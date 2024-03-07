override CFLAGS=-Wall -Wextra -Wshadow -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror
endif

.PHONY: clean all

all: sop-roulette

sop-lotto: sop-roulette.c pipe-utils.h
	gcc $(CFLAGS) -o sop-roulette sop-roulette.c

clean:
	rm -f sop-roulette
