SHELL = sh
CFLAGS = -g -pedantic -Wall -Werror -DNDEBUG

gctest: main.o log.o sym.o
	$(CC) -o $@ $^

main.o: main.c
log.o: log.c
sym.o: sym.c

.PHONY: clean
clean:
	rm -f gctest *.o
