
CC	= gcc
LD	= ld

TOPDIR		= `pwd`
CFLAGS_SDL	= `pkg-config --cflags sdl`
LDFLAGS_SDL	= `pkg-config --libs sdl`
LDFLAGS_X264	= -lx264
LDFLAGS_FFMPEG	= `pkg-config --libs libavformat libavutil libavcodec libswscale`

CFLAGS	:= -g -Wall
#CFLAGS	+= -Werror -Wcast-align
CFLAGS	+= -I$(TOPDIR)
CFLAGS	+= -I./source
CFLAGS	+= -I./sink
CFLAGS	+= -I../libraries
CFLAGS	+= $(CFLAGS_SDL)

LDFLAGS	:=
LDFLAGS	+= ../libraries/libskt/libskt.a
#LDFLAGS	+= ../libraries/libmempool/libmempool.a
#LDFLAGS	+= ../libraries/libthreadpool/libthreadpool.a
LDFLAGS	+= -lpthread
LDFLAGS	+= $(LDFLAGS_SDL)
LDFLAGS	+= $(LDFLAGS_X264)
LDFLAGS += $(LDFLAGS_FFMPEG)
#LDFLAGS += -Wl,-rpath=/usr/local/lib -lrt

.PHONY : all clean

TGT	:= sms

SOURCE_OBJS	= source/source.o
SOURCE_OBJS	+= source/v4l.o
SOURCE_OBJS	+= source/udp.o

SINK_OBJS	= sink/sink.o
SINK_OBJS	+= sink/sdl.o
SINK_OBJS	+= sink/udp.o

CODEC_OBJS	= codec/codec.o
CODEC_OBJS	+= codec/x264_enc.o
CODEC_OBJS	+= codec/avcodec_dec.o


OBJS	= $(SOURCE_OBJS) $(SINK_OBJS) $(CODEC_OBJS) sms.o common.o

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

sms: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TGT)
