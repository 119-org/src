
CC	= gcc
LD	= ld

CFLAGS_SDL	= `pkg-config --cflags sdl`
LDFLAGS_SDL	= `pkg-config --libs sdl`

CFLAGS	:= -g -Wall
#CFLAGS	+= -Werror -Wcast-align
CFLAGS	+= -I../libraries
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
#LDFLAGS	+= ../libraries/libskt/libskt.a
#LDFLAGS	+= ../libraries/libmempool/libmempool.a
#LDFLAGS	+= ../libraries/libthreadpool/libthreadpool.a
LDFLAGS	+= -lpthread
LDFLAGS	+= $(LDFLAGS_SDL)

.PHONY : all clean

TGT	:= sms

SOURCE_OBJS	= source.o
SOURCE_OBJS	+= v4l.o

SINK_OBJS	= sink.o
SINK_OBJS	+= sdl.o

OBJS	= $(SOURCE_OBJS) $(SINK_OBJS) sms.o common.o

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

sms: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TGT)
