#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "common.h"
#include "source.h"

#define REGISTER_SOURCE(x) { \
    extern struct source src_##x##_module; \
    source_register(&src_##x##_module, sizeof(src_##x##_module)); }

static struct source *first_source = NULL;

static int source_register(struct source *src, int size)
{
    struct source **p;
    if (size < sizeof(struct source)) {
        struct source *temp = (struct source *)calloc(1, sizeof(struct source));
        memcpy(temp, src, size);
        src = temp;
    }
    p = &first_source;
    while (*p != NULL) p = &(*p)->next;
    *p = src;
    src->next = NULL;
    return 0;
}

int source_register_all()
{
    static int registered;
    if (registered)
        return -1;
    registered = 1;

    REGISTER_SOURCE(v4l);
//    REGISTER_SOURCE(file);
//    REGISTER_SOURCE(rtsp);

    return 0;
}

struct source_ctx *source_init(const char *input)
{
    struct source *p;
    struct source_ctx *sc = (struct source_ctx *)calloc(1, sizeof(struct source_ctx));
    if (!sc) {
        err("malloc source context failed!\n");
        return NULL;
    }
    parse_url(&sc->url, input);

    for (p = first_source; p != NULL; p = p->next) {
        if (!strcmp(sc->url.head, p->name))
            break;
    }
    if (p == NULL) {
        err("%s protocol is not support!\n", sc->url.head);
        return NULL;
    }
    dbg("use %s source module\n", p->name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        err("malloc source priv failed!\n");
        return NULL;
    }
    return sc;
}

int source_open(struct source_ctx *src)
{
    if (-1 == src->ops->open(src, src->url.body)) {
        err("source open failed!\n");
        return -1;
    }
    return 0;
}

int source_read(struct source_ctx *src, void *buf, int len)
{
    return src->ops->read(src, buf, len);
}

int source_write(struct source_ctx *src, void *buf, int len)
{
    return src->ops->write(src, buf, len);
}

void source_deinit(struct source_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
