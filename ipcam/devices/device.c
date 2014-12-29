#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <event2/event.h>

#include "common.h"
#include "device.h"
#include "queue.h"

#define REGISTER_DEVICE(x) { \
    extern struct device ipc_##x##_device; \
    device_register(&ipc_##x##_device, sizeof(ipc_##x##_device)); }

static struct device *first_device = NULL;
static int dev_registered = 0;

static int device_register(struct device *dev, int size)
{
    struct device **d;
    if (size < sizeof(struct device)) {
        struct device *temp = (struct device *)calloc(1, sizeof(struct device));
        memcpy(temp, dev, size);
        dev = temp;
    }
    d = &first_device;
    while (*d != NULL) d = &(*d)->next;
    *d = dev;
    dev->next = NULL;
    return 0;
}

int device_register_all()
{
    if (dev_registered)
        return -1;
    dev_registered = 1;

    REGISTER_DEVICE(v4l2);

    return 0;
}

struct device_ctx *device_new(const char *input)
{
    struct device *p;
    struct device_ctx *sc = (struct device_ctx *)calloc(1, sizeof(struct device_ctx));
    if (!sc) {
        printf("malloc device context failed!\n");
        return NULL;
    }
    parse_url(&sc->url, input);

    for (p = first_device; p != NULL; p = p->next) {
        if (!strcmp(sc->url.head, p->name))
            break;
    }
    if (p == NULL) {
        printf("%s protocol is not support!\n", sc->url.head);
        return NULL;
    }
    printf("[device] %s module\n", p->name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        printf("malloc device priv failed!\n");
        return NULL;
    }
    return sc;
}

int device_open(struct device_ctx *src)
{
    if (!src->ops->open)
        return -1;
    return src->ops->open(src, src->url.body);
}

int device_read(struct device_ctx *src, void *buf, int len)
{
    if (!src->ops->read)
        return -1;
    return src->ops->read(src, buf, len);
}

int device_write(struct device_ctx *src, void *buf, int len)
{
    if (!src->ops->write)
        return -1;
    return src->ops->write(src, buf, len);
}

void device_close(struct device_ctx *dc)
{
    if (!dc->ops->close)
        return;
    return dc->ops->close(dc);
}

void device_free(struct device_ctx *sc)
{
    if (sc) {
        free(sc->priv);
        free(sc);
    }
    return;
}
