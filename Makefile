SHELL = sh
CFLAGS = 

gctest: main.o
	$(CC) -o $@ $^

main.o: main.c

.PHONY: clean
clean:
	rm -f gctest *.o
