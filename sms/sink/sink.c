#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "common.h"
#include "sink.h"

#define REGISTER_SINK(x) { \
    extern struct sink snk_##x##_module; \
    sink_register(&snk_##x##_module, sizeof(snk_##x##_module)); }

static struct sink *first_sink = NULL;

static int sink_register(struct sink *snk, int size)
{
    struct sink **p;
    if (size < sizeof(struct sink)) {
        struct sink *temp = (struct sink *)calloc(1, sizeof(struct sink));
        memcpy(temp, snk, size);
        snk= temp;
    }
    p = &first_sink;
    while (*p != NULL) p = &(*p)->next;
    *p = snk;
    snk->next = NULL;
    return 0;
}

int sink_register_all()
{
    static int snk_registered;
    if (snk_registered)
        return -1;
    snk_registered = 1;

    REGISTER_SINK(sdl);
    REGISTER_SINK(udp);
//    REGISTER_SINK(file);

    return 0;
}

struct sink_ctx *sink_init(const char *input)
{
    struct sink *p;
    struct sink_ctx *sc = (struct sink_ctx *)calloc(1, sizeof(struct sink_ctx));
    if (!sc) {
        err("malloc sink context failed!\n");
        return NULL;
    }
    parse_url(&sc->url, input);

    for (p = first_sink; p != NULL; p = p->next) {
        if (!strcmp(sc->url.head, p->name))
            break;
    }
    if (p == NULL) {
        err("%s protocol is not support!\n", sc->url.head);
        return NULL;
    }
    dbg("use %s sink module\n", p->name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        err("malloc source priv failed!\n");
        return NULL;
    }
    return sc;
}

int sink_open(struct sink_ctx *snk)
{
    if (!snk->ops->open)
        return -1;
    return snk->ops->open(snk, snk->url.body);
}

int sink_read(struct sink_ctx *snk, void *buf, int len)
{
    if (!snk->ops->read)
        return -1;
    return snk->ops->read(snk, buf, len);
}

int sink_write(struct sink_ctx *snk, void *buf, int len)
{
    if (!snk->ops->write)
        return -1;
    return snk->ops->write(snk, buf, len);
}

int sink_poll(struct sink_ctx *snk)
{
    if (!snk->ops->poll)
        return -1;
    return snk->ops->poll(snk);
}

void sink_handle(struct sink_ctx *snk)
{
    if (!snk->ops->handle)
        return NULL;
    return snk->ops->handle(snk);
}

void sink_deinit(struct sink_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
