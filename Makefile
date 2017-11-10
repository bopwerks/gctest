SHELL = sh
CFLAGS = 

gctest: main.o
	$(CC) -o $@ $^ -lm

main.o: main.c

.PHONY: clean
clean:
	rm -f gctest *.o
