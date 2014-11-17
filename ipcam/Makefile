
CC	= gcc
LD	= ld

TOPDIR		= `pwd`
CFLAGS_SDL	= `pkg-config --cflags sdl`
LDFLAGS_SDL	= `pkg-config --libs sdl`
LDFLAGS_X264	= -lx264
LDFLAGS_ALSA	= -lasound
LDFLAGS_LIBEVENT = -levent
LDFLAGS_FFMPEG	= `pkg-config --libs libavformat libavutil libavcodec libswscale`

CFLAGS	:= -g -Wall
CFLAGS	+= -Werror
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= -I./codecs
CFLAGS	+= -I./devices
CFLAGS	+= -I./protocols
CFLAGS	+= -I./buffer
CFLAGS	+= -I./utils
CFLAGS	+= -I../libraries
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
LDFLAGS	+= ../libraries/libskt/libskt.a
LDFLAGS	+= -lpthread -lrt
LDFLAGS	+= $(LDFLAGS_SDL)
LDFLAGS	+= $(LDFLAGS_X264)
LDFLAGS += $(LDFLAGS_ALSA)
LDFLAGS += $(LDFLAGS_FFMPEG)
LDFLAGS += $(LDFLAGS_LIBEVENT)

.PHONY : all clean

TGT := ipcam test_libevent

DEVICE_OBJS := \
    devices/video_capture_agent.o \
    devices/device.o \
    devices/v4l2.o \
    devices/alsa.o

PROTOCOL_OBJS := \
    protocols/protocol.o \
    protocols/udp.o \
    protocols/sdl.o \
    protocols/network_agent.o \
    protocols/display_agent.o

CODEC_OBJS := \
    codecs/codec.o \
    codecs/x264_enc.o \
    codecs/avcodec_dec.o \
    codecs/x264_encode_agent.o \
    codecs/avcodec_decode_agent.o

BUFFER_OBJS := \
    buffer/queue.o \
    buffer/buffer.o

UTILS_OBJS := \
    utils/atomic.o \
    utils/common.o

IPCAM_OBJS := \
    $(DEVICE_OBJS) \
    $(PROTOCOL_OBJS) \
    $(CODEC_OBJS) \
    $(BUFFER_OBJS) \
    $(UTILS_OBJS) \
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
