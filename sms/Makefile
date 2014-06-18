
CC	= gcc
LD	= ld

CFLAGS	:= -g -Wall -O0
CFLAGS	+= -I../libraries

LDFLAGS	:=
LDFLAGS	+= ../libraries/libskt/libskt.a
LDFLAGS	+= ../libraries/libmempool/libmempool.a
LDFLAGS	+= ../libraries/libthreadpool/libthreadpool.a
LDFLAGS	+= -lpthread

.PHONY : all clean

TGT	:= sms source sink

SRC	= $(wildcard *.c)
OBJS	= $(patsubst %.c,%.o,$(SRC))

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

sms:	sms.o
	$(CC) -o $@ $^ $(LDFLAGS)

source:	source.o
	$(CC) -o $@ $^ $(LDFLAGS)

sink:	sink.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TGT)
