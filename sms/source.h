#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct source_ctx {
    void *priv;
};

struct source {
    const char *name;
    int (*source_open)(struct source_ctx *c, const char *url);
    int (*source_read)(struct source_ctx *c, uint8_t *buf, int len);
    int (*source_write)(struct source_ctx *c, const uint8_t *buf, int len);
    void (*source_close)(struct source_ctx *c);
    int priv_size;
};

struct source_ctx *source_init(const char *input);

#ifdef __cplusplus
}
#endif
#endif
