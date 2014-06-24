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
#include "common.h"
#include "debug.h"

#define MAX_V4L_BUF		32
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
    int buf_index;
};

static int v4l_set_format(struct v4l_ctx *vc)
{
    struct v4l2_format fmt;
    struct v4l2_pix_format *pix = &fmt.fmt.pix;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_G_FMT, &fmt)) {
        perror("ioctl(VIDIOC_G_FMT)");
        goto fail;
    }
    dbg("pix.format: %d*%d\n", pix->width, pix->height);
    if (-1 == ioctl(vc->fd, VIDIOC_S_FMT, &fmt)) {
        perror("ioctl(VIDIOC_S_FMT)");
        goto fail;
    }
    vc->width = pix->width;
    vc->height = pix->height;

    return 0;
fail:
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
    if (-1 == ioctl(vc->fd, VIDIOC_REQBUFS, &req)) {
        perror("ioctl(VIDIOC_REQBUFS)");
        goto fail;
    }
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
//        printf("addr: %p len: %d\n", vc->buf[i].addr, vc->buf[i].len);
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
    dbg("v4l stream on\n");
    return 0;
fail:
    close(vc->fd);
    return -1;
}

static int v4l_buf_enqueue(struct v4l_ctx *vc)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = vc->buf_index;
    if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
        if (errno == EINVAL) {
            perror("ioctl(VIDIOC_DQBUF)");
        } else {
            perror("ioctl(VIDIOC_QBUF)");
        }
        return -1;
    }
    return 0;
}

static int v4l_buf_dequeue(struct v4l_ctx *vc, struct frame *f)
{
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(vc->fd, VIDIOC_DQBUF, &buf)) {
        if (errno == EAGAIN) {
            //buf.index = vc->buf_index;
            perror("ioctl(VIDIOC_DQBUF)");
            return 0;
        } else {
            perror("ioctl(VIDIOC_DQBUF)");
            return -1;
        }
    }
    vc->buf_index = buf.index;
    f->index = buf.index;
    f->addr = vc->buf[buf.index].addr;
    f->len = buf.bytesused;
//    dbg("source frame addr = %p, len = %d, index = %d\n", f->addr, f->len, f->index);

    return buf.index;
}

static int v4l_open(struct source_ctx *sc, const char *dev)
{
    int fd;
    struct v4l_ctx *vc = sc->priv;

//    fd = open(dev, O_RDWR | O_NONBLOCK);
    fd = open(dev, O_RDWR);
    if (fd == -1) {
        perror("open");
        err("open %s failed!\n", dev);
        return -1;
    }
    vc->fd = fd;

    v4l_set_format(vc);
    v4l_req_buf(vc);
    sc->width = vc->width;
    sc->height = vc->height;
    return 0;
}

static int v4l_read(struct source_ctx *sc, void *buf, int len)
{
    struct v4l_ctx *vc = sc->priv;
    struct frame f;
    int i;
    int ret = v4l_buf_dequeue(vc, &f);
    if (ret == -1) {
        err("v4l dequeue failed!\n");
        return -1;
    }
    for (i = 0; i < f.len; i++) {//8 byte copy
        *((char *)buf + i) = *((char *)f.addr + i);
    }
    return f.len;
}

static int v4l_write(struct source_ctx *sc, void *buf, int len)
{
    struct v4l_ctx *vc = sc->priv;
    return v4l_buf_enqueue(vc);
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
