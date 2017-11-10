SHELL = sh
CFLAGS = -g -pedantic -Wall -Werror

gctest: main.o log.o
	$(CC) -o $@ $^

main.o: main.c
log.o: log.c

.PHONY: clean
clean:
	rm -f gctest *.o
