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
#include "video_device_agent.h"

static void on_video_device_read(int fd, short what, void *arg)
{
    int ret, len;
    struct queue_item *item = NULL;
    int flen = 0x96000;//equals to one v4l2 frame buffer
    void *frm = calloc(1, flen);
    video_device_agent_t *ua = (video_device_agent_t *)arg;
    struct device_ctx *dev = ua->dev;

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
    item = queue_item_new(frm, flen);
    ret = queue_push(ua->qout, item);
    if (ret == -1) {
        printf("queue_push failed!\n");
    }
}

static void *video_device_agent_loop(void *arg)
{
    struct video_device_agent *ua = (struct video_device_agent *)arg;
    if (!ua)
        return NULL;
    event_base_loop(ua->ev_base, 0);
    return NULL;
}

static void on_break_event(int fd, short what, void *arg)
{

}

static void break_event_base_loop(struct video_device_agent *ua)
{
    char notify = '1';
    if (!ua)
        return;
    if (write(ua->pipe_wfd, &notify, sizeof(notify)) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}

struct video_device_agent *video_device_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    int fds[2];
    pthread_t tid;
    video_device_agent_t *ua;
    char *v4l2_url = "v4l2:///dev/video0";

    ua = (video_device_agent_t *)calloc(1, sizeof(video_device_agent_t));
    if (!ua)
        return NULL;

    ua->dev = device_new(v4l2_url);
    if (!ua->dev)
        return NULL;

    if (-1 == device_open(ua->dev)) {
        printf("open %s failed!\n", v4l2_url);
        return NULL;
    }

    ua->qout = qout;
    ua->ev_base = event_base_new();
    if (!ua->ev_base)
        return NULL;

    ua->ev_read = event_new(ua->ev_base, ua->dev->fd, EV_READ|EV_PERSIST, on_video_device_read, ua);
    if (!ua->ev_read) {
        return NULL;
    }
    if (-1 == event_add(ua->ev_read, NULL)) {
        return NULL;
    }

    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    ua->pipe_rfd = fds[0];
    ua->pipe_wfd = fds[1];

    ua->ev_close = event_new(ua->ev_base, ua->pipe_rfd, EV_READ|EV_PERSIST, on_break_event, ua->dev);
    if (!ua->ev_close) {
        return NULL;
    }
    if (-1 == event_add(ua->ev_close, NULL)) {
        return NULL;
    }

    if (0 != pthread_create(&tid, NULL, video_device_agent_loop, ua)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(ua);
        return NULL;
    }

    return ua;
}

void video_device_agent_destroy(struct video_device_agent *ua)
{
    if (!ua)
        return;

    if (0 != event_base_loopbreak(ua->ev_base)) {
        break_event_base_loop(ua);
    }

    event_del(ua->ev_read);
    event_del(ua->ev_close);
    event_base_free(ua->ev_base);
    device_close(ua->dev);
    device_free(ua->dev);
    queue_free(ua->qout);
    free(ua);
}
