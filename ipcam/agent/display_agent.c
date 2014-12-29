#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "yqueue.h"
#include "display.h"
#include "agent.h"
#include "common.h"


struct display_agent_ctx {
    struct display_ctx *pc;
};

static int on_display_read(void *arg, void *in_data, int in_len, void **out_data, int *out_len)
{
    struct display_agent_ctx *da = (struct display_agent_ctx *)arg;
    *out_len = display_write(da->pc, in_data, in_len);
    if (*out_len == -1) {
        //printf("display failed!\n");
    }
    //printf("%s:%d len = %d\n", __func__, __LINE__, *out_len);
    return *out_len;
}

static int display_agent_open(struct agent_ctx *ac)
{
    struct display_agent_ctx *dac = ac->priv;
    const char *url = "sdl://rgb";
    dac->pc = display_new(url);
    if (!dac->pc)
        return -1;
    int width = ac->media_info->video.width;
    int height = ac->media_info->video.height;

    printf("display_open width=%d, height=%d\n", width, height);
    if (-1 == display_open(dac->pc, width, height)) {
        printf("open %s failed!\n", url);
        return -1;
    }
    ac->rfd = -1;
    ac->wfd = -1;

    return 0;
}

static void display_agent_close(struct agent_ctx *ac)
{
    struct display_agent_ctx *dac = ac->priv;
    display_close(dac->pc);
    display_free(dac->pc);
}

struct agent ipc_display_agent = {
    .name = "display",
    .open = display_agent_open,
    .on_read = on_display_read,
    .on_write = NULL,
    .close = display_agent_close,
    .priv_size = sizeof(struct display_agent_ctx),
};
