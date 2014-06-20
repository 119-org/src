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

static int sink_register(struct sink *src, int size)
{
    struct sink **p;
    if (size < sizeof(struct sink)) {
        struct sink *temp = (struct sink *)calloc(1, sizeof(struct sink));
        memcpy(temp, src, size);
        src = temp;
    }
    p = &first_sink;
    while (*p != NULL) p = &(*p)->next;
    *p = src;
    src->next = NULL;
    return 0;
}

int sink_register_all()
{
    static int registered;
    if (registered)
        return -1;
    registered = 1;

    REGISTER_SINK(sdl);
//    REGISTER_SINK(rtsp);
//    REGISTER_SINK(file);

    return 0;
}

struct sink_ctx *sink_init(const char *input)
{
    struct url *u;
    struct sink *p;

    u = (struct url *)calloc(1, sizeof(struct url));
    parse_url(u, input);

    for (p = first_sink; p != NULL; p = p->next) {
        if (!strcmp(u->head, p->name))
            break;
    }
    if (p == NULL) {
        err("%s protocol is not support!\n", u->head);
        return NULL;
    }
    dbg("use %s sink module\n", p->name);

    struct sink_ctx *sc = (struct sink_ctx *)calloc(1, sizeof(struct sink_ctx));
    if (!sc) {
        err("malloc sink context failed!\n");
        return NULL;
    }
    sc->url = u;
    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);

    return sc;
}

void sink_deinit(struct sink_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
