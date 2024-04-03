override CFLAGS=-Wall -Wextra -fanalyzer -g -O0 -fsanitize=address,undefined -Iunity

ifdef CI
override CFLAGS=-Wall -Wextra -Werror -Iunity
endif

.PHONY: all clean clean-resources

all: sop-bridge

cards.o : cards.c cards.h
	gcc $(CFLAGS) -c -o $@ $<

hand.o: hand.c hand.h
	gcc $(CFLAGS) -c -o $@ $<

table.o: table.c table.h
	gcc $(CFLAGS) -c -o $@ $<

sop-bridge: sop-bridge.c macros.h table.o hand.o cards.o
	gcc $(CFLAGS) -o $@ $< table.o hand.o cards.o

clean:
	rm -f sop-bridge *.o

clean-resources:
	rm -f /dev/shm/sem.sop-shmem-sem /dev/shm/sop-shmem
