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
    int (*decode)(struct codec_ctx *c, void *buf, int *len, void *data);
    int priv_size;
    struct codec *next;
};

int codec_register_all();

#ifdef __cplusplus
}
#endif
#endif
