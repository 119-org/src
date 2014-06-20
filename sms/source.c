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
    struct url *u;
    struct source *p;

    u = (struct url *)calloc(1, sizeof(struct url));
    parse_url(u, input);

    for (p = first_source; p != NULL; p = p->next) {
        if (!strcmp(u->head, p->name))
            break;
    }
    if (p == NULL) {
        err("%s protocol is not support!\n", u->head);
        return NULL;
    }
    dbg("use %s source module\n", p->name);

    struct source_ctx *sc = (struct source_ctx *)calloc(1, sizeof(struct source_ctx));
    if (!sc) {
        err("malloc source context failed!\n");
        return NULL;
    }
    sc->url = u;
    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);

    return sc;
}

void source_deinit(struct source_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
