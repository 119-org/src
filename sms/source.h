#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct source_ctx {
    struct source *ops;
    struct url *url;
    void *priv;
};

struct source {
    const char *name;
    int (*open)(struct source_ctx *c, const char *url);
    int (*read)(struct source_ctx *c, uint8_t *buf, int len);
    int (*write)(struct source_ctx *c, const uint8_t *buf, int len);
    void (*close)(struct source_ctx *c);
    int priv_size;
    struct source *next;
};

struct source_ctx *source_init(const char *url);
void source_deinit(struct source_ctx *sc);
int source_register_all();

#ifdef __cplusplus
}
#endif
#endif
