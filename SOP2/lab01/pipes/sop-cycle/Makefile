override CFLAGS=-Wall -Wextra -Wshadow -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror
endif

.PHONY: clean all

all: sop-cycle

sop-lotto: sop-cycle.c pipe-utils.h
	gcc $(CFLAGS) -o sop-cycle sop-cycle.c

clean:
	rm -f sop-cycle
