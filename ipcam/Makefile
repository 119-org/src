
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
#CFLAGS	+= -Werror
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= -I./codecs
CFLAGS	+= -I./devices
CFLAGS	+= -I./protocols
CFLAGS	+= -I./queue
CFLAGS	+= -I../libraries
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
LDFLAGS	+= ../libraries/libskt/libskt.a
LDFLAGS	+= -lpthread
LDFLAGS	+= $(LDFLAGS_SDL)
LDFLAGS	+= $(LDFLAGS_X264)
LDFLAGS += $(LDFLAGS_ALSA)
LDFLAGS += $(LDFLAGS_FFMPEG)
LDFLAGS += $(LDFLAGS_LIBEVENT)

.PHONY : all clean

TGT := ipcam

DEVICE_OBJS := \
    devices/device.o \
    devices/v4l2.o \
    devices/usbcam_agent.o \
    devices/alsa.o

PROTOCOL_OBJS := \
    protocols/protocol.o \
    protocols/udp.o \
    protocols/sdl.o

CODEC_OBJS := \
    codecs/codec.o \
    codecs/x264_enc.o \
    codecs/avcodec_dec.o \
    codecs/x264_agent.o

QUEUE_OBJS := \
    queue/queue.o

OBJS := \
    $(DEVICE_OBJS) \
    $(PROTOCOL_OBJS) \
    $(CODEC_OBJS) \
    $(QUEUE_OBJS) \
    common.o \
    main.o 

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TGT)
