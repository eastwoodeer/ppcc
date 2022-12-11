CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
CC=clang

ppcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): ppcc.h

test: ppcc
	./test.sh

clean:
	rm -f *.o *.s ppcc tmp

.PHONY: clean test
