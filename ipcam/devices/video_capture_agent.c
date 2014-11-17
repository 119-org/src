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
#include "buffer.h"
#include "video_capture_agent.h"

static void on_video_capture_read(int fd, short what, void *arg)
{
    int ret, len;
    video_capture_agent_t *vda = (video_capture_agent_t *)arg;
    struct buffer_item *item = NULL;
    int flen = 2 * (vda->dev->video.width) * (vda->dev->video.height);//YUV422 2*w*h
    void *frm = calloc(1, flen);
    struct device_ctx *dev = vda->dev;

    len = device_read(dev, frm, flen);
    if (len == -1) {
        printf("device_read failed!\n");
        free(frm);
        if (-1 == device_write(dev, NULL, 0)) {
            printf("device_write failed!\n");
        }
        return;
    }
    if (-1 == device_write(dev, NULL, 0)) {
        printf("device_write failed!\n");
    }
    item = buffer_item_new(frm, flen);
    ret = buffer_push(vda->buf_snk, item);
    if (ret == -1) {
        printf("buffer_push failed!\n");
    }
}

static void *video_capture_agent_loop(void *arg)
{
    struct video_capture_agent *vda = (struct video_capture_agent *)arg;
    event_base_loop(vda->ev_base, 0);
    return NULL;
}

static void on_break_event(int fd, short what, void *arg)
{

}

static void break_event_base_loop(struct video_capture_agent *vca)
{
    char notify = '1';
    if (!vca)
        return;
    if (write(vca->pipe_wfd, &notify, sizeof(notify)) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}

struct video_capture_agent *video_capture_agent_create(struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk)
{
    int fds[2];
    video_capture_agent_t *vca;
    char *v4l2_url = "v4l2:///dev/video0";

    vca = (video_capture_agent_t *)calloc(1, sizeof(video_capture_agent_t));
    if (!vca)
        return NULL;

    vca->dev = device_new(v4l2_url);
    if (!vca->dev)
        return NULL;

    if (-1 == device_open(vca->dev)) {
        printf("open %s failed!\n", v4l2_url);
        return NULL;
    }

    vca->buf_src = buf_src;
    vca->buf_snk = buf_snk;
    vca->ev_base = event_base_new();
    if (!vca->ev_base)
        return NULL;

    vca->ev_read = event_new(vca->ev_base, vca->dev->fd, EV_READ|EV_PERSIST, on_video_capture_read, vca);
    if (!vca->ev_read) {
        return NULL;
    }
    if (-1 == event_add(vca->ev_read, NULL)) {
        return NULL;
    }

    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    vca->pipe_rfd = fds[0];
    vca->pipe_wfd = fds[1];

    vca->ev_close = event_new(vca->ev_base, vca->pipe_rfd, EV_READ|EV_PERSIST, on_break_event, vca->dev);
    if (!vca->ev_close) {
        return NULL;
    }
    if (-1 == event_add(vca->ev_close, NULL)) {
        return NULL;
    }

    return vca;
}

int video_capture_agent_dispatch(struct video_capture_agent *vca)
{
    pthread_t tid;

    if (0 != pthread_create(&tid, NULL, video_capture_agent_loop, vca)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(vca);
        return -1;
    }
    return 0;
}


void video_capture_agent_destroy(struct video_capture_agent *vca)
{
    if (!vca)
        return;

    if (0 != event_base_loopbreak(vca->ev_base)) {
        break_event_base_loop(vca);
    }

    event_del(vca->ev_read);
    event_del(vca->ev_close);
    event_base_free(vca->ev_base);
    device_close(vca->dev);
    device_free(vca->dev);
    free(vca);
}
