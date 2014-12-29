
CC	= gcc
LD	= ld

TOPDIR		= `pwd`
CFLAGS_SDL	= `pkg-config --cflags sdl`
LDFLAGS_SDL	= `pkg-config --libs sdl`
LDFLAGS_X264	= -lx264
LDFLAGS_LIBEVENT = -levent -levent_pthreads
LDFLAGS_FFMPEG	= `pkg-config --libs libavformat libavutil libavcodec libswscale`

CFLAGS	:= -g -Wall
CFLAGS	+= -Werror
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= -I./utils
CFLAGS	+= -I./buffer
CFLAGS	+= -I./protocol
CFLAGS	+= -I./codec
CFLAGS	+= -I./display
CFLAGS	+= -I./buffer
CFLAGS	+= -I./agent
CFLAGS	+= -I./utils
CFLAGS	+= -I../libraries
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
LDFLAGS	+= ../libraries/libskt/libskt.a
LDFLAGS	+= ../libraries/libptcp/libptcp.a
LDFLAGS	+= -lpthread -lrt
LDFLAGS	+= $(LDFLAGS_SDL)
LDFLAGS	+= $(LDFLAGS_X264)
LDFLAGS += $(LDFLAGS_ALSA)
LDFLAGS += $(LDFLAGS_FFMPEG)
LDFLAGS += $(LDFLAGS_LIBEVENT)

.PHONY : all clean

TGT	:= player

UTILS_OBJS := \
    utils/atomic.o \
    utils/common.o

BUFFER_OBJS := \
    buffer/queue.o \
    buffer/yqueue.o \
    buffer/buffer.o


PROTOCOL_OBJS := \
    protocol/protocol.o \
    protocol/udp.o \
    protocol/tcp.o \
    protocol/ptcp.o

DECODE_OBJS := \
    codec/codec.o \
    codec/h264_dec.o

DISPLAY_OBJS := \
    display/display.o \
    display/sdl.o

AGENT_OBJS := \
    agent/agent.o \
    agent/downstream_agent.o \
    agent/vdecode_agent.o \
    agent/display_agent.o

PLAYER_OBJS := \
    $(UTILS_OBJS) \
    $(BUFFER_OBJS) \
    $(PROTOCOL_OBJS) \
    $(DECODE_OBJS) \
    $(DISPLAY_OBJS) \
    $(AGENT_OBJS) \
    main.o

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

player: $(PLAYER_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PLAYER_OBJS) $(TGT)
