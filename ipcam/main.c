#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include "device.h"
#include "protocol.h"
#include "codec.h"
#include "queue.h"
#include "video_device_agent.h"
#include "x264_agent.h"
#include "network_agent.h"
#include "decoder_agent.h"
#include "display_agent.h"

typedef struct ipcam {
    struct device_ctx *dev;
    struct protocol_ctx *prt;
    struct codec_ctx *encoder;
    struct codec_ctx *decoder;
    struct video_device_agent *ua;
    struct x264_agent *xa;
    struct network_agent *na;
    struct display_agent *da;

} ipcam_t;

static struct ipcam *ipcam_instance = NULL;

struct ipcam *ipcam_init()
{
    ipcam_t *ipcam = NULL;
    struct video_device_agent *ua = NULL;
    struct x264_agent *xa = NULL;
    struct network_agent *na = NULL;
    struct queue_ctx *dev_qout = NULL;
    struct queue_ctx *enc_qout = NULL;
    struct queue_ctx *dec_qout = NULL;

    device_register_all();
    protocol_register_all();
    codec_register_all();

    ipcam = (ipcam_t *)calloc(1, sizeof(ipcam_t));
    if (!ipcam)
        return NULL;
    dev_qout = queue_new(5);
    enc_qout = queue_new(5);
    dec_qout = queue_new(5);

    ipcam->ua = video_device_agent_create(NULL, dev_qout);
    if (!ipcam->ua) {
        printf("usbcam_agent_create failed!\n");
        return NULL;
    }
#if 1
    ipcam->xa = x264_agent_create(dev_qout, enc_qout);
    if (!ipcam->xa) {
        printf("x264_agent_create failed!\n");
        return NULL;
    }
#endif
#if 0
    ipcam->na = network_agent_create(enc_qout, NULL);
    if (!ipcam->na) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
#endif
#if 1
    ipcam->na = decoder_agent_create(enc_qout, dec_qout);
    if (!ipcam->na) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
#endif
#if 1
    ipcam->da = display_agent_create(dec_qout, NULL);
    if (!ipcam->da) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
#endif
#if 0
    ipcam->ua = ua;
    ipcam->dev = device_new("v4l2:///dev/video0");
    ipcam->prt = protocol_init("sdl://player");
//    ipcam->prt = protocol_init("udp://127.0.0.1:2333");
    ipcam->encoder = codec_init("x264");
    ipcam->decoder = codec_init("avcodec");
#endif

    return ipcam;
}

int ipcam_open()
{
#if 0
    struct device_ctx *dev = ipcam_instance->dev;
    struct protocol_ctx *prt = ipcam_instance->prt;
    struct codec_ctx *encoder = ipcam_instance->encoder;
    struct codec_ctx *decoder = ipcam_instance->decoder;

    if (-1 == device_open(dev)) {
        err("source_open failed!\n");
        return -1;
    }
    if (-1 == codec_open(encoder, dev->width, dev->height)) {
        err("codec_open failed!\n");
        return -1;
    }
    if (-1 == codec_open(decoder, dev->width, dev->height)) {
        err("codec_open failed!\n");
        return -1;
    }
    if (-1 == protocol_open(prt)) {
        err("sink_open failed!\n");
        return -1;
    }
#endif
}

void ipcam_dispatch()
{
    struct device_ctx *dev = ipcam_instance->dev;
    struct protocol_ctx *prt = ipcam_instance->prt;
    struct codec_ctx *encoder = ipcam_instance->encoder;
    struct codec_ctx *decoder = ipcam_instance->decoder;
    int ret, len;
    int flen = 0x100000;
    void *frm = calloc(1, flen);
    void *pkt = calloc(1, flen);
    void *yuv = frm;
    int quit = 0;

    while (!quit) {
        len = device_read(dev, frm, flen);
        if (len == -1) {
            err("source read failed!\n");
            continue;
        }
#if 0
        len = codec_encode(encoder, frm, pkt);
        if (len == -1) {
            err("encode failed!\n");
            continue;
        }
        ret = codec_decode(decoder, pkt, len, &yuv);
        if (ret == -1) {
            err("decode failed!\n");
            continue;
        }
        ret == protocol_write(prt, yuv, len);
        if (ret == -1) {
            err("sink write failed!\n");
            continue;
        }
#endif
        printf("write %d\n", ret);
        if (-1 == device_write(dev, NULL, 0)) {
            err("source write failed!\n");
            continue;
        }
    }

}

static void sigterm_handler(int sig)
{
    video_device_agent_destroy(ipcam_instance->ua);
    x264_agent_destroy(ipcam_instance->xa);
    network_agent_destroy(ipcam_instance->na);
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, sigterm_handler);

    ipcam_instance = ipcam_init();
    if (!ipcam_instance) {
        return -1;
    }

    while (1) {
        sleep(2);
    }
#if 0
    ipcam_open();
    ipcam_dispatch();
#endif

}
