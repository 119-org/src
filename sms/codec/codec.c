#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "debug.h"
#include "common.h"
#include "codec.h"

#define SMS_REGISTER_ENCODER(x) { \
    extern struct codec cdc_##x##_encoder; \
    codec_register(&cdc_##x##_encoder, sizeof(cdc_##x##_encoder)); }

#define SMS_REGISTER_DECODER(x) { \
    extern struct codec cdc_##x##_decoder; \
    codec_register(&cdc_##x##_decoder, sizeof(cdc_##x##_decoder)); }

static struct codec *first_codec = NULL;

static int codec_register(struct codec *cdc, int size)
{
    struct codec **p;
    if (size < sizeof(struct codec)) {
        struct codec *temp = (struct codec *)calloc(1, sizeof(struct codec));
        memcpy(temp, cdc, size);
        cdc= temp;
    }
    p = &first_codec;
    while (*p != NULL) p = &(*p)->next;
    *p = cdc;
    cdc->next = NULL;
    return 0;
}

int codec_register_all()
{
    static int cdc_registered;
    if (cdc_registered)
        return -1;
    cdc_registered = 1;

    SMS_REGISTER_ENCODER(x264);
    SMS_REGISTER_DECODER(avcodec);

//    REGISTER_DECODER(x264);
    return 0;
}

struct codec_ctx *codec_init(const char *name)
{
    struct codec *p;
    struct codec_ctx *c = (struct codec_ctx *)calloc(1, sizeof(struct codec_ctx));
    if (!c) {
        err("malloc codec context failed!\n");
        return NULL;
    }
    for (p = first_codec; p != NULL; p = p->next) {
        if (!strcmp(name, p->name))
            break;
    }
    if (p == NULL) {
        err("%s codec is not support!\n", name);
        return NULL;
    }
    dbg("use %s codec module\n", p->name);
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
    if (!c->ops->encode) {
        dbg("xxx\n");
        return -1;
    }
    return c->ops->encode(c, in, out);
}

int codec_decode(struct codec_ctx *c, void *in, int inlen, void **out)
{
    if (!c->ops->decode)
        return -1;
    return c->ops->decode(c, in, inlen, out);
}

void codec_deinit(struct codec_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
