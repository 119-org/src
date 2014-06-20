#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "source.h"
#include "debug.h"

#define MAX_V4L_BUF	4
#define MAX_V4L_DEV		4
#define MAX_V4L_REQBUF_CNT	256

struct v4l_buf {
    void *addr;
    int len;
};

struct v4l_ctx {
    int fd;
    int width;
    int height;
    struct v4l_buf buf[MAX_V4L_BUF];
};

struct v4l_frame {
    void *addr;
    int len;
    int index;
};

static int v4l_set_format(struct v4l_ctx *vc)
{
    struct v4l2_format fmt;
    struct v4l2_pix_format *pix = &fmt.fmt.pix;

    dbg("fd = %d\n", vc->fd);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_G_FMT, &fmt)) {
        perror("ioctl(VIDIOC_G_FMT)");
        goto fail;
    }
    dbg("pix.width: %d\n"
        "pix.height: %d\n"
        "pix.pixelformat: %d\n",
        pix->width,
        pix->height,
        pix->pixelformat);

    if (-1 == ioctl(vc->fd, VIDIOC_S_FMT, &fmt)) {
        perror("ioctl(VIDIOC_S_FMT)");
        goto fail;
    }

fail:
    dbg("fd = %d\n", vc->fd);
    close(vc->fd);
    return -1;
}

static int v4l_req_buf(struct v4l_ctx *vc)
{
    int i;
    struct v4l2_requestbuffers req;
    enum v4l2_buf_type type;

    memset(&req, 0, sizeof(req));
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.count = MAX_V4L_REQBUF_CNT;
    req.memory = V4L2_MEMORY_MMAP;
//request buffer
    dbg("fd = %d\n", vc->fd);
    if (-1 == ioctl(vc->fd, VIDIOC_REQBUFS, &req)) {
        perror("ioctl(VIDIOC_REQBUFS)");
        goto fail;
    }
    printf("req count is %d\n", req.count);
    assert(req.count <= MAX_V4L_REQBUF_CNT);
    for (i = 0; i < (int)req.count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
//query buffer
        if (-1 == ioctl(vc->fd, VIDIOC_QUERYBUF, &buf)) {
            perror("ioctl(VIDIOC_QUERYBUF)");
            goto fail;
        }

//mmap buffer
        vc->buf[i].len = buf.length;
        vc->buf[i].addr = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                               vc->fd, buf.m.offset);
        if (MAP_FAILED == vc->buf[i].addr) {
            perror("mmap");
            goto fail;
        }

//enqueue buffer
        printf("addr: %p len: %d\n", vc->buf[i].addr, vc->buf[i].len);
        if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
            perror("ioctl(VIDIOC_QBUF)");
            goto fail;
        }
    }

//stream on
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_STREAMON, &type)) {
        perror("ioctl(VIDIOC_STREAMON)");
        goto fail;
    }
    return 0;
fail:
    close(vc->fd);
    dbg("fd = %d\n", vc->fd);
    return -1;
}

static int v4l_buf_enqueue(struct v4l_ctx *vc, struct v4l_frame *vf)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = vf->index;
    if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
        perror("ioctl(VIDIOC_QBUF)");
        return -1;
    }
    return 0;
}

static int v4l_buf_dequeue(struct v4l_ctx *vc, struct v4l_frame *vf)
{
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(vc->fd, VIDIOC_DQBUF, &buf)) {
        perror("ioctl(VIDIOC_DQBUF)");
        return -1;
    }
    vf->addr = vc->buf[buf.index].addr;
    vf->len = buf.bytesused;
    vf->index = buf.index;
    return buf.index;
}

static int v4l_open(struct source_ctx *sc, const char *dev)
{
    int i, fd;
    int valid = 4;
    char device[13];
    struct v4l_ctx *vc = sc->priv;

    fd = open(dev, O_RDWR | O_NONBLOCK, 0);
    if (fd == -1) {
        perror("open");
        err("open %s failed!\n", dev);
        return -1;
    }
    dbg("open %s success.\n", dev);
    vc->fd = fd;
    dbg("fd = %d\n", vc->fd);

    v4l_set_format(vc);
    v4l_req_buf(vc);
    return 0;
}

static int v4l_read(struct source_ctx *sc, uint8_t *buf, int len)
{
    struct v4l_ctx *vc = sc->priv;

    return 0;
}

static int v4l_write(struct source_ctx *sc, const uint8_t *buf, int len)
{
    struct v4l_ctx *vc = sc->priv;

    return 0;
}

static void v4l_close(struct source_ctx *sc)
{
    struct v4l_ctx *vc = sc->priv;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == ioctl(vc->fd, VIDIOC_STREAMOFF, &type)) {
        if (errno == EINVAL) {
            perror("streaming i/o is not support");
        } else {
            perror("ioctl(VIDIOC_STREAMOFF)");
        }
    }
    close(vc->fd);
}

struct source src_v4l_module = {
    .name = "v4l",
    .open = v4l_open,
    .read = v4l_read,
    .write = v4l_write,
    .close = v4l_close,
    .priv_size = sizeof(struct v4l_ctx),
};
