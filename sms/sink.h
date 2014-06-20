#ifndef _SINK_H_
#define _SINK_H_

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sink_ctx {
    struct sink *ops;
    struct url *url;
    void *priv;
};

struct sink {
    const char *name;
    int (*open)(struct sink_ctx *c);
    int (*read)(struct sink_ctx *c, uint8_t *buf, int len);
    int (*write)(struct sink_ctx *c, uint8_t *buf, int len);
    void (*close)(struct sink_ctx *c);
    int priv_size;
    struct sink *next;
};

struct sink_ctx *sink_init(const char *url);
void sink_deinit(struct sink_ctx *sc);
int sink_register_all();


#ifdef __cplusplus
}
#endif
#endif
