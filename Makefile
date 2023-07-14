CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: shell

nyush: shell.c


.PHONY: clean
clean:
	rm -f *.o shell