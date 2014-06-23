#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "common.h"
#include "codec.h"

#define REGISTER_ENCODER(x) { \
    extern struct codec cdc_##x##_encoder; \
    codec_register(&cdc_##x##_encoder, sizeof(cdc_##x##_encoder)); }

#define REGISTER_DECODER(x) { \
    extern struct codec cdc_##x##_decoder; \
    codec_register(&cdc_##x##_decoder, sizeof(cdc_##x##_decoder)); }

static struct codec *first_codec = NULL;

static int codec_register(struct codec *src, int size)
{
    struct codec **p;
    if (size < sizeof(struct codec)) {
        struct codec *temp = (struct codec *)calloc(1, sizeof(struct codec));
        memcpy(temp, src, size);
        src = temp;
    }
    p = &first_codec;
    while (*p != NULL) p = &(*p)->next;
    *p = src;
    src->next = NULL;
    return 0;
}

int codec_register_all()
{
    static int registered;
    if (registered)
        return -1;
    registered = 1;

    REGISTER_ENCODER(x264);

//    REGISTER_DECODER(x264);
    return 0;
}

struct codec_ctx *codec_init(const char *name)
{
    struct codec *p;
    struct codec_ctx *c = (struct codec_ctx *)calloc(1, sizeof(struct codec_ctx));

    for (p = first_codec; p != NULL; p = p->next) {
        if (!strcmp(name, p->name))
            break;
    }
    if (p == NULL) {
        err("%s codec is not support!\n", name);
        return NULL;
    }
    c->ops = p;
    c->priv = calloc(1, p->priv_size);
    if (!c->priv) {
        err("malloc codec priv failed!\n");
        return NULL;
    }
    return c;
}

int codec_open(struct codec_ctx *c, int width, int height)
{
    if (!c->ops->open)
        return -1;
    return c->ops->open(c, width, height);
}

int codec_encode(struct codec_ctx *c, void *in, void *out)
{
    if (!c->ops->encode)
        return -1;
    return c->ops->encode(c, in, out);
}


void codec_deinit(struct codec_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
