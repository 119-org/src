#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "common.h"
#include "protocol.h"

#define REGISTER_PROTOCOL(x) { \
    extern struct protocol ipc_##x##_protocol; \
    protocol_register(&ipc_##x##_protocol, sizeof(ipc_##x##_protocol)); }

static struct protocol *first_protocol = NULL;
static int prt_registered = 0;

static int protocol_register(struct protocol *prt, int size)
{
    struct protocol **p;
    if (size < sizeof(struct protocol)) {
        struct protocol *temp = (struct protocol *)calloc(1, sizeof(struct protocol));
        memcpy(temp, prt, size);
        prt= temp;
    }
    p = &first_protocol;
    while (*p != NULL) p = &(*p)->next;
    *p = prt;
    prt->next = NULL;
    return 0;
}

int protocol_register_all()
{
    if (prt_registered)
        return -1;
    prt_registered = 1;

    REGISTER_PROTOCOL(udp);
    REGISTER_PROTOCOL(sdl);

    return 0;
}

struct protocol_ctx *protocol_init(const char *input)
{
    struct protocol *p;
    struct protocol_ctx *sc = (struct protocol_ctx *)calloc(1, sizeof(struct protocol_ctx));
    if (!sc) {
        err("malloc protocol context failed!\n");
        return NULL;
    }
    parse_url(&sc->url, input);

    for (p = first_protocol; p != NULL; p = p->next) {
        if (!strcmp(sc->url.head, p->name))
            break;
    }
    if (p == NULL) {
        err("%s protocol is not support!\n", sc->url.head);
        return NULL;
    }
    dbg("use %s protocol module\n", p->name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        err("malloc source priv failed!\n");
        return NULL;
    }
    return sc;
}

int protocol_open(struct protocol_ctx *snk)
{
    if (!snk->ops->open)
        return -1;
    return snk->ops->open(snk, snk->url.body);
}

int protocol_read(struct protocol_ctx *snk, void *buf, int len)
{
    if (!snk->ops->read)
        return -1;
    return snk->ops->read(snk, buf, len);
}

int protocol_write(struct protocol_ctx *snk, void *buf, int len)
{
    if (!snk->ops->write)
        return -1;
    return snk->ops->write(snk, buf, len);
}

int protocol_poll(struct protocol_ctx *snk)
{
    if (!snk->ops->poll)
        return -1;
    return snk->ops->poll(snk);
}

void protocol_handle(struct protocol_ctx *snk)
{
    if (!snk->ops->handle)
        return;
    return snk->ops->handle(snk);
}

void protocol_deinit(struct protocol_ctx *sc)
{
    if (!sc)
        return;
    free(sc);
}
