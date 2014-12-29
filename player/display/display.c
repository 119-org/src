#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "common.h"
#include "display.h"

#define REGISTER_DISPLAY(x) { \
    extern struct display mp_##x##_display; \
    display_register(&mp_##x##_display, sizeof(mp_##x##_display)); }

static struct display *first_display = NULL;
static int prt_registered = 0;

static int display_register(struct display *prt, int size)
{
    struct display **p;
    if (size < sizeof(struct display)) {
        struct display *temp = (struct display *)calloc(1, sizeof(struct display));
        memcpy(temp, prt, size);
        prt= temp;
    }
    p = &first_display;
    while (*p != NULL) p = &(*p)->next;
    *p = prt;
    prt->next = NULL;
    return 0;
}

int display_register_all()
{
    if (prt_registered)
        return -1;
    prt_registered = 1;

    REGISTER_DISPLAY(sdl);

    return 0;
}

struct display_ctx *display_new(const char *input)
{
    struct display *p;
    struct display_ctx *sc = (struct display_ctx *)calloc(1, sizeof(struct display_ctx));
    if (!sc) {
        return NULL;
    }
    parse_url(&sc->url, input);

    for (p = first_display; p != NULL; p = p->next) {
        if (!strcmp(sc->url.head, p->name))
            break;
    }
    if (p == NULL) {
        printf("%s display is not support!\n", sc->url.head);
        return NULL;
    }
    printf("[display] %s module\n", p->name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        return NULL;
    }
    return sc;
}

int display_open(struct display_ctx *c, int width, int height)
{
    if (!c->ops->open)
        return -1;
    return c->ops->open(c, c->url.body, width, height);
}

int display_read(struct display_ctx *c, void *buf, int len)
{
    if (!c->ops->read)
        return -1;
    return c->ops->read(c, buf, len);
}

int display_write(struct display_ctx *c, void *buf, int len)
{
    if (!c->ops->write)
        return -1;
    return c->ops->write(c, buf, len);
}

void display_close(struct display_ctx *c)
{
    if (!c->ops->close)
        return;
    return c->ops->close(c);
}

void display_free(struct display_ctx *sc)
{
    if (!sc)
        return;
    free(sc->priv);
    free(sc);
}
