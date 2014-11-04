#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct device_ctx {
    int fd;
    struct url url;
    int width;
    int height;
    struct device *ops;
    void *priv;
};

struct device {
    const char *name;
    int (*open)(struct device_ctx *c, const char *url);
    int (*read)(struct device_ctx *c, void *buf, int len);
    int (*write)(struct device_ctx *c, void *buf, int len);
    void (*close)(struct device_ctx *c);
    int priv_size;
    struct device *next;
};

struct device_ctx *device_new(const char *url);
void device_free(struct device_ctx *sc);
int device_open(struct device_ctx *src);
int device_read(struct device_ctx *src, void *buf, int len);
int device_write(struct device_ctx *src, void *buf, int len);
int device_register_all();

#ifdef __cplusplus
}
#endif
#endif
