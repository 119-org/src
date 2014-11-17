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

struct protocol_ctx *protocol_new(const char *input)
{
    struct protocol *p;
    struct protocol_ctx *sc = (struct protocol_ctx *)calloc(1, sizeof(struct protocol_ctx));
    if (!sc) {
        return NULL;
    }
    parse_url(&sc->url, input);

    for (p = first_protocol; p != NULL; p = p->next) {
        if (!strcmp(sc->url.head, p->name))
            break;
    }
    if (p == NULL) {
        printf("%s protocol is not support!\n", sc->url.head);
        return NULL;
    }
    printf("use %s protocol module\n", p->name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        return NULL;
    }
    return sc;
}

int protocol_open(struct protocol_ctx *c)
{
    if (!c->ops->open)
        return -1;
    return c->ops->open(c, c->url.body);
}

int protocol_read(struct protocol_ctx *c, void *buf, int len)
{
    if (!c->ops->read)
        return -1;
    return c->ops->read(c, buf, len);
}

int protocol_write(struct protocol_ctx *c, void *buf, int len)
{
    if (!c->ops->write)
        return -1;
    return c->ops->write(c, buf, len);
}

int protocol_poll(struct protocol_ctx *c)
{
    if (!c->ops->poll)
        return -1;
    return c->ops->poll(c);
}

void protocol_handle(struct protocol_ctx *c)
{
    if (!c->ops->handle)
        return;
    return c->ops->handle(c);
}

void protocol_close(struct protocol_ctx *c)
{
    if (!c->ops->close)
        return;
    return c->ops->close(c);
}

void protocol_free(struct protocol_ctx *sc)
{
    if (!sc)
        return;
    free(sc);
}
