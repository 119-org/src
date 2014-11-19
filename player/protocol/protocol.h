#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct protocol_ctx {
    int fd;
    struct url url;
    struct protocol *ops;
    void *priv;
};

struct protocol {
    const char *name;
    int (*open)(struct protocol_ctx *c, const char *url);
    int (*read)(struct protocol_ctx *c, void *buf, int len);
    int (*write)(struct protocol_ctx *c, void *buf, int len);
    void (*close)(struct protocol_ctx *c);
    int (*poll)(struct protocol_ctx *c);
    void (*handle)(struct protocol_ctx *c);
    int priv_size;
    struct protocol *next;
};

struct protocol_ctx *protocol_new(const char *url);
void protocol_free(struct protocol_ctx *c);

int protocol_open(struct protocol_ctx *c);
void protocol_close(struct protocol_ctx *c);

int protocol_read(struct protocol_ctx *c, void *buf, int len);
int protocol_write(struct protocol_ctx *c, void *buf, int len);

int protocol_poll(struct protocol_ctx *c);
void protocol_handle(struct protocol_ctx *c);

int protocol_register_all();

#ifdef __cplusplus
}
#endif
#endif
