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

struct upstream_agent_ctx {
    struct protocol_ctx *pc;

};

static int on_upstream_read(void *arg, void *in_data, int in_len, void **out_data, int *out_len)
{
    struct upstream_agent_ctx *xa = (struct upstream_agent_ctx *)arg;
    struct protocol_ctx *pc = xa->pc;

    *out_len = protocol_write(pc, in_data, in_len);
    if (*out_len == -1) {
        printf("protocol_write failed!\n");
    }
    printf("%s:%d len = %d\n", __func__, __LINE__, *out_len);
    return *out_len;
}


static int upstream_open(struct agent_ctx *ac)
{
    struct upstream_agent_ctx *c = ac->priv;
    c->pc = protocol_new("udp://192.168.1.110:2333");
    if (!c->pc) {
        return -1;
    }
    if (-1 == protocol_open(c->pc)) {
        return -1;
    }
    ac->rfd = -1;
    ac->wfd = -1;
    return 0;
}

static void upstream_close(struct agent_ctx *ac)
{
    struct upstream_agent_ctx *c = ac->priv;
    protocol_close(c->pc);
    protocol_free(c->pc);
}

struct agent ipc_upstream_agent = {
    .name = "upstream",
    .open = upstream_open,
    .on_read = on_upstream_read,
    .on_write = NULL,
    .close = upstream_close,
    .priv_size = sizeof(struct upstream_agent_ctx),
};
