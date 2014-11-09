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
#include "usbcam_agent.h"



typedef void (*timer_handle_t)(union sigval sv);

timer_t timer_handle_create(timer_handle_t cb, void *arg, struct timeval *tv)
{
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    memset(&sev, 0, sizeof(struct sigevent));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = cb;
    sev.sigev_value.sival_ptr = arg;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        printf("timer_create");
        return -1;
    }

    its.it_value.tv_sec = tv->tv_sec;
    its.it_value.tv_nsec = tv->tv_usec * 1000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        printf("timer_settime");
        return -1;
    }
    return timerid;
}


static void on_usbcam_write(union sigval sv)
{
    struct device_ctx *dc = (struct device_ctx *)sv.sival_ptr;
    if (-1 == device_write(dc, NULL, 0)) {
        printf("on_device_write failed!\n");
        return;
    }
    return;
}

static void on_usbcam_read(int fd, short what, void *arg)
{
    int len;
    struct queue_item *item = NULL;
    int flen = 0x96000;//equals to one v4l2 frame buffer
    void *frm = calloc(1, flen);
    usbcam_agent_t *ua = (usbcam_agent_t *)arg;
    struct device_ctx *dc = ua->dc;
    len = device_read(dc, frm, flen);
    if (len == -1) {
        free(frm);
        printf("source read failed!\n");
        return;
    }
    item = queue_item_new(frm, flen);
    queue_add(ua->qout, item);
}

static void *usbcam_agent_loop(void *arg)
{
    struct usbcam_agent *ua = (struct usbcam_agent *)arg;
    if (!ua)
        return NULL;
    event_base_loop(ua->ev_base, 0);
    return NULL;
}

static void on_break_event(int fd, short what, void *arg)
{

}



void timer_handle_destroy(timer_t tid)
{
    timer_delete(tid);
}

void notify_to_break_event_loop(struct usbcam_agent *ua)
{
    char notify = '1';
    if (!ua)
        return;
    if (write(ua->on_write_fd, &notify, 1) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}

struct usbcam_agent *usbcam_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    struct event_base *ev_base = NULL;
    struct event *ev_read = NULL;
    struct event *ev_close = NULL;
    usbcam_agent_t *ua = NULL;
    struct device_ctx *v4l2_dev = NULL;
    struct timeval tv = {0, 150*1000};
    timer_t timerid;
    pthread_t tid;
    int fds[2];

    ua = (usbcam_agent_t *)calloc(1, sizeof(usbcam_agent_t));
    if (!ua)
        return NULL;

    v4l2_dev = device_new("v4l2:///dev/video0");
    if (!v4l2_dev)
        return NULL;
    if (-1 == device_open(v4l2_dev)) {
        printf("source_open failed!\n");
        return -1;
    }
    ev_base = event_base_new();
    if (!ev_base)
        return NULL;

    timerid = timer_handle_create(on_usbcam_write, v4l2_dev, &tv);
    if (-1 == timerid) {
        return NULL;
    }

    ev_read = event_new(ev_base, v4l2_dev->fd, EV_READ|EV_PERSIST, on_usbcam_read, ua);
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

    ev_close = event_new(ev_base, fds[0], EV_READ|EV_PERSIST, on_break_event, v4l2_dev);
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
    ua->timerid = timerid;
    ua->dc = v4l2_dev;
    ua->qout = qout;

    if (0 != pthread_create(&tid, NULL, usbcam_agent_loop, ua)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(ua);
        return NULL;
    }

    return ua;
}

void usbcam_agent_destroy(struct usbcam_agent *ua)
{
    if (!ua)
        return;

    timer_handle_destroy(ua->timerid);

    if (0 != event_base_loopbreak(ua->ev_base)) {
        notify_to_break_event_loop(ua);
    }

    event_del(ua->ev_read);
    event_del(ua->ev_close);
    event_base_free(ua->ev_base);
    device_close(ua->dc);
    device_free(ua->dc);
    queue_free(ua->qout);
    free(ua);
}
