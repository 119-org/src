
CC	= gcc
LD	= ld

TOPDIR		= `pwd`
CFLAGS_SDL	= `pkg-config --cflags sdl`
LDFLAGS_SDL	= `pkg-config --libs sdl`
LDFLAGS_X264	= -lx264
LDFLAGS_LIBEVENT= -levent
LDFLAGS_FFMPEG	= `pkg-config --libs libavformat libavutil libavcodec libswscale`

CFLAGS	:= -g -Wall
#CFLAGS	+= -Werror -Wcast-align
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
LDFLAGS	+= -lpthread
LDFLAGS	+= $(LDFLAGS_SDL)
LDFLAGS	+= $(LDFLAGS_X264)
LDFLAGS += $(LDFLAGS_FFMPEG)
LDFLAGS += $(LDFLAGS_LIBEVENT)
LDFLAGS += -Wl,-rpath=/usr/local/lib -lrt

.PHONY : all clean

TGT	:= player

OBJS	= player.o

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

player: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TGT)
