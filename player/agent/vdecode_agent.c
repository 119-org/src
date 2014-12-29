#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "agent.h"
#include "common.h"
#include "codec.h"
#include "buffer.h"

struct vdecode_agent_ctx {
    struct codec_ctx *vdec;
};

static int on_vdecode_read(void *arg, void *in_data, int in_len, void **out_data, int *out_len)
{
    struct vdecode_agent_ctx *c = (struct vdecode_agent_ctx *)arg;
    *out_len = codec_decode(c->vdec, in_data, in_len, out_data);
    if (*out_len == -1) {
        printf("decode failed!\n");
    }
    //printf("%s:%d len = %d\n", __func__, __LINE__, *out_len);
    return *out_len;
}

static int vdecode_agent_open(struct agent_ctx *ac)
{
    struct vdecode_agent_ctx *c = ac->priv;
    c->vdec = codec_new("h264dec");
    if (!c->vdec)
        return -1;
    int width = ac->media_info->video.width;
    int height = ac->media_info->video.height;

    printf("vdecode_agent_open width=%d, height=%d\n", width, height);
    if (-1 == codec_open(c->vdec, width, height)) {
        printf("open avcodec failed!\n");
        return -1;
    }
    ac->rfd = -1;
    ac->wfd = -1;

    return 0;
}

static void vdecode_agent_close(struct agent_ctx *ac)
{
    struct vdecode_agent_ctx *c = ac->priv;
    codec_close(c->vdec);
    codec_free(c->vdec);
}

struct agent mp_vdecode_agent = {
    .name = "vdecode",
    .open = vdecode_agent_open,
    .on_read = on_vdecode_read,
    .on_write = NULL,
    .close = vdecode_agent_close,
    .priv_size = sizeof(struct vdecode_agent_ctx),
};
