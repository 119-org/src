#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "yqueue.h"
#include "device.h"
#include "agent.h"
#include "common.h"

struct videocap_agent_ctx {
    struct device_ctx *dev;
};

static int on_videocap_read(void *arg, void *in_data, int in_len, void **out_data, int *out_len)
{
    struct videocap_agent_ctx *ctx = (struct videocap_agent_ctx *)arg;
    int flen = 2 * (ctx->dev->video.width) * (ctx->dev->video.height);//YUV422 2*w*h
    void *frm = calloc(1, flen);

    *out_len = device_read(ctx->dev, frm, flen);
    if (*out_len == -1) {
        printf("device_read failed!\n");
        free(frm);
        if (-1 == device_write(ctx->dev, NULL, 0)) {
            printf("device_write failed!\n");
        }
        return -1;
    }
    if (-1 == device_write(ctx->dev, NULL, 0)) {
        printf("device_write failed!\n");
    }
    *out_data = frm;
    //printf("%s:%d len = %d\n", __func__, __LINE__, *out_len);
    return *out_len;
}

static int videocap_open(struct agent_ctx *ac)
{
    struct videocap_agent_ctx *vac = ac->priv;
    char *v4l2_url = "v4l2:///dev/video0";
    vac->dev = device_new(v4l2_url);
    if (!vac->dev)
        return -1;

    if (-1 == device_open(vac->dev)) {
        printf("open %s failed!\n", v4l2_url);
        return -1;
    }
    ac->rfd = vac->dev->fd;
    ac->wfd = -1;
    ac->media_info->video.width = vac->dev->video.width;
    ac->media_info->video.height = vac->dev->video.height;

    return 0;
}

static void videocap_close(struct agent_ctx *ac)
{
    struct videocap_agent_ctx *vac = ac->priv;
    device_close(vac->dev);
    device_free(vac->dev);
}

struct agent ipc_videocap_agent = {
    .name = "videocap",
    .open = videocap_open,
    .on_read = on_videocap_read,
    .on_write = NULL,
    .close = videocap_close,
    .priv_size = sizeof(struct videocap_agent_ctx),
};
