#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "yqueue.h"
#include "protocol.h"
#include "agent.h"
#include "common.h"

struct downstream_agent_ctx {
    struct protocol_ctx *pc;

};

static int on_downstream_read(void *arg, void *in_data, int in_len, void **out_data, int *out_len)
{
    struct downstream_agent_ctx *xa = (struct downstream_agent_ctx *)arg;
    struct protocol_ctx *pc = xa->pc;
    *out_len = 6500;
    *out_data = calloc(1, *out_len);
    *out_len = protocol_read(pc, *out_data, *out_len);
    if (*out_len == -1) {
        printf("protocol_write failed!\n");
    }
    printf("%s:%d len = %d\n", __func__, __LINE__, *out_len);
    return *out_len;
}

static int downstream_open(struct agent_ctx *ac)
{
    struct downstream_agent_ctx *c = ac->priv;
    c->pc = protocol_new("udp://192.168.1.110:2333");
    if (!c->pc) {
        return -1;
    }
    if (-1 == protocol_open(c->pc)) {
        return -1;
    }
    ac->rfd = c->pc->fd;
    ac->wfd = -1;
    ac->media_info->video.width = 320;
    ac->media_info->video.height = 240;

    return 0;
}

static void downstream_close(struct agent_ctx *ac)
{
    struct downstream_agent_ctx *c = ac->priv;
    protocol_close(c->pc);
    protocol_free(c->pc);
}

struct agent mp_downstream_agent = {
    .name = "downstream",
    .open = downstream_open,
    .on_read = on_downstream_read,
    .on_write = NULL,
    .close = downstream_close,
    .priv_size = sizeof(struct downstream_agent_ctx),
};
