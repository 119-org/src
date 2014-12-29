
CC	= gcc
LD	= ld

TOPDIR		= `pwd`
CFLAGS_SDL	= `pkg-config --cflags sdl`
LDFLAGS_SDL	= `pkg-config --libs sdl`
LDFLAGS_X264	= -lx264
LDFLAGS_ALSA	= -lasound
LDFLAGS_LIBEVENT = -levent -levent_pthreads
LDFLAGS_FFMPEG	= `pkg-config --libs libavformat libavutil libavcodec libswscale`

CFLAGS	:= -g -Wall
CFLAGS	+= -Werror
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= -I./codec
CFLAGS	+= -I./device
CFLAGS	+= -I./protocol
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

TGT := test_libevent ipcam 

DEVICE_OBJS := \
    device/device.o \
    device/v4l2.o \
    device/alsa.o

PROTOCOL_OBJS := \
    protocol/protocol.o \
    protocol/udp.o \
    protocol/tcp.o \
    protocol/ptcp.o

DISPLAY_OBJS := \
    display/display.o \
    display/sdl.o

CODEC_OBJS := \
    codec/codec.o \
    codec/x264_enc.o \
    codec/avcodec_dec.o

BUFFER_OBJS := \
    buffer/queue.o \
    buffer/yqueue.o \
    buffer/buffer.o

UTILS_OBJS := \
    utils/atomic.o \
    utils/common.o

AGENT_OBJS := \
    agent/agent.o \
    agent/videocap_agent.o \
    agent/x264enc_agent.o \
    agent/display_agent.o \
    agent/upstream_agent.o

IPCAM_OBJS := \
    $(DEVICE_OBJS) \
    $(PROTOCOL_OBJS) \
    $(DISPLAY_OBJS) \
    $(CODEC_OBJS) \
    $(BUFFER_OBJS) \
    $(UTILS_OBJS) \
    $(AGENT_OBJS) \
    main.o 

UNITTEST_OBJS := \
    test/test_libevent.o

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

ipcam: $(IPCAM_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_libevent: $(UNITTEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TGT) $(IPCAM_OBJS) $(UNITTEST_OBJS)
