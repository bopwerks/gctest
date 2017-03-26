SHELL = sh

debug: CFLAGS=-DNDEBUG
debug: gctest

gctest: main.o
	$(CC) -o $@ $^

main.o: main.c

.PHONY: clean
clean:
	rm -f gctest *.o
