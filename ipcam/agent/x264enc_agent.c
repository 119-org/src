#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "yqueue.h"
#include "codec.h"
#include "agent.h"

struct x264enc_agent_ctx {
    struct codec_ctx *encoder;
};

static int on_x264enc_read(void *arg, void *in_data, int in_len, void **out_data, int *out_len)
{
    struct x264enc_agent_ctx *xa = (struct x264enc_agent_ctx *)arg;
    struct codec_ctx *encoder = xa->encoder;

    *out_len = codec_encode(encoder, in_data, out_data);
    if (*out_len == -1) {
        printf("encode failed!\n");
    }
    //printf("%s:%d len = %d\n", __func__, __LINE__, *out_len);
    return *out_len;
}

static int x264enc_open(struct agent_ctx *ac)
{
    struct x264enc_agent_ctx *xac = ac->priv;
    xac->encoder = codec_new("x264");
    if (!xac->encoder)
        return -1;
    int width = ac->media_info->video.width;
    int height = ac->media_info->video.height;

    printf("x264enc_open width=%d, height=%d\n", width, height);
    if (-1 == codec_open(xac->encoder, width, height)) {
        printf("open x264 failed!\n");
        return -1;
    }
    ac->rfd = -1;
    ac->wfd = -1;

    return 0;
}

static void x264enc_close(struct agent_ctx *ac)
{
    struct x264enc_agent_ctx *xac = ac->priv;
    codec_close(xac->encoder);
    codec_free(xac->encoder);
}

struct agent ipc_x264enc_agent = {
    .name = "x264enc",
    .open = x264enc_open,
    .on_read = on_x264enc_read,
    .on_write = NULL,
    .close = x264enc_close,
    .priv_size = sizeof(struct x264enc_agent_ctx),
};
