override CFLAGS=-Wall -Wextra -Wshadow -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror
endif

.PHONY: clean all

all: sop-lotto

sop-lotto: sop-lotto.c pipe-utils.h
	gcc $(CFLAGS) -o sop-lotto sop-lotto.c

clean:
	rm -f sop-lotto
