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

#include "device.h"
//#include "common.h"
//#include "debug.h"

#define MAX_V4L_BUF		32
#define MAX_V4L_REQBUF_CNT	256

struct v4l2_buf {
    void *addr;
    int len;
};

struct v4l2_ctx {
    int fd;
    int on_read_fd;
    int on_write_fd;
    int width;
    int height;
    struct v4l2_buf buf[MAX_V4L_BUF];
    int buf_index;
};

static int v4l2_set_format(struct v4l2_ctx *vc)
{
    struct v4l2_format fmt;
    struct v4l2_pix_format *pix = &fmt.fmt.pix;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_G_FMT, &fmt)) {
        printf("ioctl(VIDIOC_G_FMT) failed: %s\n", strerror(errno));
        return -1;
    }
    printf("pix.format: %d*%d\n", pix->width, pix->height);
    if (-1 == ioctl(vc->fd, VIDIOC_S_FMT, &fmt)) {
        printf("ioctl(VIDIOC_S_FMT) failed: %s\n", strerror(errno));
        return -1;
    }
    vc->width = pix->width;
    vc->height = pix->height;

    return 0;
}

static int v4l2_buf_enqueue(struct v4l2_ctx *vc)
{
    struct v4l2_buffer buf;
    char notify = '1';
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = vc->buf_index;

    if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
        printf("ioctl(VIDIOC_QBUF): %s\n", strerror(errno));
        return -1;
    }
    if (write(vc->on_write_fd, &notify, 1) != 1) {
        printf("Failed writing to notify pipe: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int v4l2_req_buf(struct v4l2_ctx *vc)
{
    int i;
    struct v4l2_requestbuffers req;
    enum v4l2_buf_type type;
    char notify = '1';

    memset(&req, 0, sizeof(req));
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.count = MAX_V4L_REQBUF_CNT;
    req.memory = V4L2_MEMORY_MMAP;
    //request buffer
    if (-1 == ioctl(vc->fd, VIDIOC_REQBUFS, &req)) {
        printf("ioctl(VIDIOC_REQBUFS): %s\n", strerror(errno));
        return -1;
    }
    if (req.count > MAX_V4L_REQBUF_CNT || req.count < 2) {
        printf("Insufficient buffer memory\n");
        return -1;
    }
    for (i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        //query buffer
        if (-1 == ioctl(vc->fd, VIDIOC_QUERYBUF, &buf)) {
            printf("ioctl(VIDIOC_QUERYBUF): %s\n", strerror(errno));
            return -1;
        }

        //mmap buffer
        vc->buf[i].len = buf.length;
        vc->buf[i].addr = mmap(NULL, buf.length, PROT_READ|PROT_WRITE,
                               MAP_SHARED, vc->fd, buf.m.offset);
        if (MAP_FAILED == vc->buf[i].addr) {
            printf("mmap failed: %s\n", strerror(errno));
            return -1;
        }

        //enqueue buffer
        if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
            printf("ioctl(VIDIOC_QBUF): %s\n", strerror(errno));
            return -1;
        }
    }

    //stream on
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_STREAMON, &type)) {
        printf("ioctl(VIDIOC_STREAMON): %s\n", strerror(errno));
        return -1;
    }
    if (write(vc->on_write_fd, &notify, 1) != 1) {
        printf("Failed writing to notify pipe\n");
        return -1;
    }
    return 0;
}

static int v4l2_buf_dequeue(struct v4l2_ctx *vc, struct frame *f)
{
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    while (1) {
        if (-1 == ioctl(vc->fd, VIDIOC_DQBUF, &buf)) {
            perror("ioctl(VIDIOC_DQBUF)");
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN)
                return -1;
        }
        break;
    }
    vc->buf_index = buf.index;
    f->index = buf.index;
    f->addr = vc->buf[buf.index].addr;
    f->len = buf.bytesused;
//    printf("v4l2 frame buffer addr = %p, len = %d, index = %d\n", f->addr, f->len, f->index);

    return f->len;
}

static int v4l2_open(struct device_ctx *dc, const char *dev)
{
    int fd;
    int fds[2];
    struct v4l2_ctx *vc = (struct v4l2_ctx *)dc->priv;

    if (pipe(fds)) {
        perror("Can't create notify pipe");
        return -1;
    }
    vc->on_read_fd = fds[0];
    vc->on_write_fd = fds[1];

    fd = open(dev, O_RDWR);
    if (fd == -1) {
        printf("open %s failed: %s\n", dev, strerror(errno));
        return -1;
    }
    vc->fd = fd;

    if (-1 == v4l2_set_format(vc)) {
        printf("v4l2_set_format failed\n");
        goto fail;
    }
    if (-1 == v4l2_req_buf(vc)) {
        printf("v4l2_req_buf failed\n");
        goto fail;
    }

    dc->fd = vc->on_read_fd;
    dc->video.width = vc->width;
    dc->video.height = vc->height;
    return 0;

fail:
    close(fd);
    return -1;
}

static int v4l2_read(struct device_ctx *dc, void *buf, int len)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)dc->priv;
    struct frame f;
    int i, flen;
    char notify;

    if (read(vc->on_read_fd, &notify, sizeof(notify)) != 1) {
        perror("Failed read from notify pipe");
    }

    flen = v4l2_buf_dequeue(vc, &f);
    if (flen == -1) {
        printf("v4l2 dequeue failed!\n");
        return -1;
    }
    if (flen > len) {
        printf("v4l2 frame is %d bytes, but buffer len is %d, not enough!\n", flen, len);
        return -1;
    }
    printf("buffer len = %d, f.len = %d\n", len, f.len);
    assert(len>=f.len);

    for (i = 0; i < f.len; i++) {//8 byte copy
        *((char *)buf + i) = *((char *)f.addr + i);
    }
    return f.len;
}

static int v4l2_write(struct device_ctx *dc, void *buf, int len)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)dc->priv;
    return v4l2_buf_enqueue(vc);
}

static void v4l2_close(struct device_ctx *dc)
{
    int i;
    struct v4l2_ctx *vc = (struct v4l2_ctx *)dc->priv;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == ioctl(vc->fd, VIDIOC_STREAMOFF, &type)) {
        printf("ioctl(VIDIOC_STREAMOFF) failed: %s\n", strerror(errno));
    }
    for (i = 0; i < MAX_V4L_REQBUF_CNT; i++) {
        munmap(vc->buf[i].addr, vc->buf[i].len);
    }
    close(vc->fd);
    close(vc->on_read_fd);
    close(vc->on_write_fd);
}

struct device ipc_v4l2_device = {
    .name = "v4l2",
    .open = v4l2_open,
    .read = v4l2_read,
    .write = v4l2_write,
    .close = v4l2_close,
    .priv_size = sizeof(struct v4l2_ctx),
};
