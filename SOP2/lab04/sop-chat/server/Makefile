override CFLAGS=-Wall -Wextra -Wshadow -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror
endif

.PHONY: clean all

all: sop-chat

sop-chat: sop-chat.c
	gcc $(CFLAGS) -o sop-chat sop-chat.c

clean:
	rm -f sop-chat
