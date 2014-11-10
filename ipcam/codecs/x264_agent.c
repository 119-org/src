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
#include "codec.h"
#include "queue.h"
#include "x264_agent.h"



static void on_break_event(int fd, short what, void *arg)
{

}

static void on_x264_read(int fd, short what, void *arg)
{
    int len;
    struct queue_item *item = NULL;
    int flen = 0x96000;//equals to one v4l2 frame buffer
    void *frm = calloc(1, flen);
    x264_agent_t *ua = (x264_agent_t *)arg;
    struct codec_ctx *encoder = ua->dc;
        len = codec_encode(encoder, ua->qin->data, ua->qout->data);
        if (len == -1) {
            err("encode failed!\n");
            continue;
        }
    len = device_read(dc, frm, flen);
    if (len == -1) {
        free(frm);
        printf("source read failed!\n");
        return;
    }
    item = queue_item_new(frm, flen);
    queue_add(ua->qout, item);
}

static void *x264_agent_loop(void *arg)
{
    struct x264_agent *ua = (struct x264_agent *)arg;
    if (!ua)
        return NULL;
    event_base_loop(ua->ev_base, 0);
    return NULL;
}

struct x264_agent *x264_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    struct event_base *ev_base = NULL;
    struct event *ev_read = NULL;
    struct event *ev_close = NULL;
    x264_agent_t *ua = NULL;
    struct codec_ctx *x264_cdc = NULL;
    struct timeval tv = {0, 150*1000};
    pthread_t tid;
    int fds[2];

    ua = (x264_agent_t *)calloc(1, sizeof(x264_agent_t));
    if (!ua)
        return NULL;

    x264_cdc = codec_new("x264");
    if (!x264_cdc)
        return NULL;
    if (-1 == codec_open(x264_cdc, 640, 480)) {
        printf("source_open failed!\n");
        return -1;
    }
    ua->dc = x264_cdc;
    ua->qin = qin;
    ua->qout = qout;


    ev_base = event_base_new();
    if (!ev_base)
        return NULL;

    ev_read = event_new(ev_base, qin->on_read_fd, EV_READ|EV_PERSIST, on_x264_read, ua);
    if (!ev_read) {
        return NULL;
    }
    if (-1 == event_add(ev_read, NULL)) {
        return NULL;
    }

    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }

    ev_close = event_new(ev_base, fds[0], EV_READ|EV_PERSIST, on_break_event, x264_cdc);
    if (!ev_close) {
        return NULL;
    }
    if (-1 == event_add(ev_close, NULL)) {
        return NULL;
    }
    ua->on_read_fd = fds[0];
    ua->on_write_fd = fds[1];

    ua->ev_read = ev_read;
    ua->ev_base = ev_base;

    if (0 != pthread_create(&tid, NULL, x264_agent_loop, ua)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(ua);
        return NULL;
    }

    return ua;
}

static void notify_to_break_event_loop(struct x264_agent *ua)
{
    char notify = '1';
    if (!ua)
        return;
    if (write(ua->on_write_fd, &notify, 1) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}


void x264_agent_destroy(struct x264_agent *ua)
{
    if (!ua)
        return;

    if (0 != event_base_loopbreak(ua->ev_base)) {
        notify_to_break_event_loop(ua);
    }

    event_del(ua->ev_read);
    event_del(ua->ev_close);
    event_base_free(ua->ev_base);
    device_close(ua->dc);
    device_free(ua->dc);
    queue_free(ua->qin);
    queue_free(ua->qout);
    free(ua);
}
