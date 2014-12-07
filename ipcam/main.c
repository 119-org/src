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
#include "buffer.h"
#include "video_capture_agent.h"
#include "x264_encode_agent.h"
#include "network_agent.h"
#include "avcodec_decode_agent.h"
#include "display_agent.h"

#define TEST_DISPLAY 0
typedef struct ipcam {
    struct video_capture_agent *vca;
    struct x264_encode_agent *xea;
    struct avcodec_decode_agent *avda;
    struct network_agent *na;
    struct display_agent *da;
} ipcam_t;

static struct ipcam *ipcam_instance = NULL;

struct ipcam *ipcam_init()
{
    ipcam_t *ipcam = NULL;

    device_register_all();
    protocol_register_all();
    codec_register_all();

    ipcam = (ipcam_t *)calloc(1, sizeof(ipcam_t));
    if (!ipcam)
        return NULL;

    struct buffer_ctx *dev_buf_src = NULL;
    struct buffer_ctx *dev_buf_snk = buffer_create(3);
    ipcam->vca = video_capture_agent_create(dev_buf_src, dev_buf_snk);
    if (!ipcam->vca) {
        printf("usbcam_agent_create failed!\n");
        return NULL;
    }

    struct buffer_ctx *enc_buf_src = dev_buf_snk;
    struct buffer_ctx *enc_buf_snk = buffer_create(4);
    ipcam->xea = x264_encode_agent_create(ipcam->vca, enc_buf_src, enc_buf_snk);
    if (!ipcam->xea) {
        printf("x264_agent_create failed!\n");
        return NULL;
    }

#if TEST_DISPLAY
    struct buffer_ctx *dec_buf_src = enc_buf_snk;
    struct buffer_ctx *dec_buf_snk = buffer_create(3);
    ipcam->avda = avcodec_decode_agent_create(dec_buf_src, dec_buf_snk);
    if (!ipcam->avda) {
        printf("network_agent_create failed!\n");
        return NULL;
    }

    struct buffer_ctx *dsp_buf_src = dec_buf_snk;
    struct buffer_ctx *dsp_buf_snk = NULL;
    ipcam->da = display_agent_create(dsp_buf_src, dsp_buf_snk);
    if (!ipcam->da) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
#else
    struct buffer_ctx *net_buf_src = enc_buf_snk;
    struct buffer_ctx *net_buf_snk = NULL;
    ipcam->na = network_agent_create(net_buf_src, net_buf_snk);
    if (!ipcam->na) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
#endif
    return ipcam;
}

void ipcam_dispatch(struct ipcam *ipc)
{
    video_capture_agent_dispatch(ipc->vca);
    x264_encode_agent_dispatch(ipc->xea);
    network_agent_dispatch(ipc->na);
    while (1) {
        sleep(2);
    }
}

static void sigterm_handler(int sig)
{
    video_capture_agent_destroy(ipcam_instance->vca);
    x264_encode_agent_destroy(ipcam_instance->xea);
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

    ipcam_dispatch(ipcam_instance);
    return 0;
}
