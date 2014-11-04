#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <event2/event.h>

#include "debug.h"
#include "common.h"
#include "device.h"

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
    printf("use %s device module\n", p->name);

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


typedef struct usbcam_agent {
    struct device_ctx *dc;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    timer_t timerid;
    int on_read_fd;
    int on_write_fd;

} usbcam_agent_t;

static void peer_keep_alive(union sigval sv)
{

}

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
    int flen = 0x96000;//equals to one v4l2 frame buffer
    void *frm = calloc(1, flen);
    struct device_ctx *dc = (struct device_ctx *)arg;
    len = device_read(dc, frm, flen);
    if (len == -1) {
        free(frm);
        printf("source read failed!\n");
        return;
    }
    free(frm);
}

static void *usbcam_agent_loop(void *arg)
{
    struct usbcam_agent *ua = (struct usbcam_agent *)arg;
    if (!ua)
        return;
    event_base_loop(ua->ev_base, 0);
}

static void on_break_event(int fd, short what, void *arg)
{

}

struct usbcam_agent *usbcam_agent_create()
{
    struct event_base *ev_base = NULL;
    struct event *ev_read = NULL;
    struct event *ev_close = NULL;
    struct event *evtimer = NULL;
    usbcam_agent_t *ua = NULL;
    struct device_ctx *v4l2_dev = NULL;
    struct timeval tv = {0, 150*1000};
    timer_t timerid;
    pthread_t tid;
    int fds[2];

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

    ev_read = event_new(ev_base, v4l2_dev->fd, EV_READ|EV_PERSIST, on_usbcam_read, v4l2_dev);
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

    ua = (usbcam_agent_t *)calloc(1, sizeof(usbcam_agent_t));
    if (!ua)
        return NULL;

    ua->on_read_fd = fds[0];
    ua->on_write_fd = fds[1];

    ua->ev_read = ev_read;
    ua->ev_base = ev_base;
    ua->timerid = timerid;
    ua->dc = v4l2_dev;

    if (0 != pthread_create(&tid, NULL, usbcam_agent_loop, ua)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(ua);
        return NULL;
    }

    return ua;
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
    free(ua);
}
