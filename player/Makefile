
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
CFLAGS	+= -I./utils
CFLAGS	+= -I./buffer
CFLAGS	+= -I./protocol
CFLAGS	+= -I./decode
CFLAGS	+= -I./display
CFLAGS	+= -I../libraries
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
LDFLAGS	+= ../libraries/libskt/libskt.a
LDFLAGS	+= ../libraries/libptcp/libptcp.a
LDFLAGS	+= -lpthread -lrt
LDFLAGS	+= $(LDFLAGS_SDL)
LDFLAGS	+= $(LDFLAGS_X264)
LDFLAGS += $(LDFLAGS_FFMPEG)
LDFLAGS += $(LDFLAGS_LIBEVENT)
LDFLAGS += -Wl,-rpath=/usr/local/lib -lrt

.PHONY : all clean

TGT	:= player

UTILS_OBJS := \
    utils/atomic.o \
    utils/common.o

BUFFER_OBJS := \
    buffer/buffer.o

PROTOCOL_OBJS := \
    protocol/protocol.o \
    protocol/udp.o \
    protocol/ptcp.o \
    protocol/network_agent.o \

DECODE_OBJS := \
    decode/codec.o \
    decode/avcodec_dec.o \
    decode/avcodec_decode_agent.o

DISPLAY_OBJS := \
    display/sdl.o \
    display/display_agent.o

PLAYER_OBJS := \
    $(UTILS_OBJS) \
    $(BUFFER_OBJS) \
    $(PROTOCOL_OBJS) \
    $(DECODE_OBJS) \
    $(DISPLAY_OBJS) \
    main.o

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

player: $(PLAYER_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PLAYER_OBJS) $(TGT)
