#ifndef _SINK_H_
#define _SINK_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


struct sink_ctx {

    void *priv;
    int priv_size;
};

struct sink {
    const char *name;
    int (*sink_open)(struct sink_ctx *c);
    int (*sink_read)(struct sink_ctx *c, uint8_t *buf, int len);
    int (*sink_write)(struct sink_ctx *c, uint8_t *buf, int len);
    void (*sink_close)(struct sink_ctx *c);
    int priv_size;
};


#ifdef __cplusplus
}
#endif
#endif
