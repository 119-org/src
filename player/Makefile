
CC	= gcc
LD	= ld

TOPDIR		= `pwd`
CFLAGS_SDL2	= `pkg-config --cflags sdl2`
LDFLAGS_SDL2	= `pkg-config --libs sdl2`
LDFLAGS_X264	= -lx264
LDFLAGS_LIBEVENT= -levent
LDFLAGS_FFMPEG	= `pkg-config --libs libavformat libavutil libavcodec libswscale`

CFLAGS	:= -g -Wall
#CFLAGS	+= -Werror -Wcast-align
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= $(CFLAGS_SDL2)

LDFLAGS	:=
LDFLAGS	+= -lpthread
LDFLAGS	+= $(LDFLAGS_SDL2)
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
