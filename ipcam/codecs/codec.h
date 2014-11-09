#ifndef _CODEC_H_
#define _CODEC_H_

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct codec_ctx {
    struct codec *ops;
    void *priv;
};

struct codec {
    const char *name;
    int (*open)(struct codec_ctx *c, int width, int height);
    int (*encode)(struct codec_ctx *c, void *in, void *out);
    int (*decode)(struct codec_ctx *c, void *in, int inlen, void **out);
    void (*close)(struct codec_ctx *c);
    int priv_size;
    struct codec *next;
};

struct codec_ctx *codec_init(const char *name);
void codec_deinit(struct codec_ctx *sc);
int codec_open(struct codec_ctx *src, int w, int h);
int codec_encode(struct codec_ctx *c, void *in, void *out);
int codec_decode(struct codec_ctx *c, void *in, int inlen, void **out);
int codec_register_all();


int codec_register_all();

#ifdef __cplusplus
}
#endif
#endif
