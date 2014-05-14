CC	= g++

CFLAGS	= -g -Wall
LDFLAGS	= -lpthread


all: libstun.a test

%.o:%.cc
	$(CC) -c $(CFLAGS) $< -o $@

libstun.a: libstun.o
	ar rc $@ $^

test: test.o
	$(CC) -o $@ $^ libstun.a $(LDFLAGS)

clean:
	rm -f *.o libstun.a test
