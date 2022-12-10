CFLAGS=-std=c11 -g -fno-common
CC=clang

ppcc: main.c
	$(CC) -o ppcc main.c $(LDFLAGS)

test: ppcc
	./test.sh

clean:
	rm -f *.o *.s ppcc tmp

.PHONY: clean test
