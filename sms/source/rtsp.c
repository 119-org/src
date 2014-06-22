#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "source.h"
#include "common.h"
#include "debug.h"

struct rtsp_ctx {

};

int rtsp_open(struct source_ctx *sc, const char *url)
{
    struct rtsp_ctx *rc = sc->priv;

}
int rtsp_read(struct source_ctx *sc, void *buf, int len)
{
    struct rtsp_ctx *rc = sc->priv;

}
int rtsp_write(struct source_ctx *sc, void *buf, int len)
{
    struct rtsp_ctx *rc = sc->priv;

}
void rtsp_close(struct source_ctx *sc)
{
    struct rtsp_ctx *rc = sc->priv;

}

struct source src_rtsp_module = {
    .name = "rtsp",
    .open = rtsp_open,
    .read = rtsp_read,
    .write = rtsp_write,
    .close = rtsp_close,
    .priv_size = sizeof(struct rtsp_ctx),
};
