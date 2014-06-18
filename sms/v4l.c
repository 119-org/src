#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define MAX_V4L_DEV	4
#define MAX_V4L_BUF	4

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


int v4l_open(struct v4l_ctx *vc)
{
    int i, fd;
    int valid = 4;
    char device[13];

    for (i = 0; i < valid; i++) {
        sprintf(device, "%s%d", "/dev/video", i);
        fd = open(device, O_RDWR | O_NONBLOCK, 0);
        if (fd == -1) {
            fprintf(stderr, "open %s failed!\n", device);
            continue;
        }
        break;
    }
    if (i == valid) {
        perror("open video device");
        return -1;
    }
    vc->fd = fd;
    return 0;
}

int v4l_set_format(struct v4l_ctx *vc)
{
    int ret;
    struct v4l2_format fmt;

    if (-1 == ioctl(vc->fd, VIDIOC_G_FMT, &fmt)) {
        perror("ioctl(VIDIOC_G_FMT)");
        goto fail;
    }

    if (-1 == ioctl(vc->fd, VIDIOC_S_FMT, &fmt)) {
        perror("ioctl(VIDIOC_S_FMT)");
        goto fail;
    }

fail:
    close(vc->fd);
    return -1;
}
