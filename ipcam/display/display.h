#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct display_ctx {
    struct url url;
    struct display *ops;
    void *priv;
};

struct display {
    const char *name;
    int (*open)(struct display_ctx *c, const char *url, int width, int height);
    int (*read)(struct display_ctx *c, void *buf, int len);
    int (*write)(struct display_ctx *c, void *buf, int len);
    void (*close)(struct display_ctx *c);
    int priv_size;
    struct display *next;
};

struct display_ctx *display_new(const char *url);
void display_free(struct display_ctx *c);

int display_open(struct display_ctx *c, int width, int height);
void display_close(struct display_ctx *c);

int display_read(struct display_ctx *c, void *buf, int len);
int display_write(struct display_ctx *c, void *buf, int len);

int display_register_all();

#ifdef __cplusplus
}
#endif
#endif
