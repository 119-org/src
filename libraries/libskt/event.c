#include <stdio.h>
#include <stdlib.h>
#include "event.h"

extern const struct eventop selectops;
extern const struct eventop epollops;

static const struct eventop *eventops[] = {
//    &selectops,
    &epollops,
};

typedef void (event_cb)(int fd, short flags, void *args);

struct event_base *event_init()
{
    int i;
    struct event_base *e = (struct event_base *)calloc(1, sizeof(struct event_base));
    if (!e) {
        fprintf(stderr, "malloc failed!\n");
        return NULL;
    }
    e->base = NULL;

    for (i = 0; eventops[i] && !e->base; i++) {
        e->base = eventops[i]->init();
        e->evop = eventops[i];
    }
    INIT_LIST_HEAD(&e->head);

    return e;
}

int event_add(struct event_base *eb, const struct timeval *tv, int fd, short flags, event_cb *cb, void *arg)
{
    struct event *ev = calloc(1, sizeof(struct event));
    if (NULL == ev) {
        return -1;
    }
    ev->evbase = eb;
    ev->evfd = fd;
    ev->evcb.cb = cb;
    ev->evcb.arg = arg;
    ev->evcb.flags = flags;

    list_add(&ev->entry, &eb->head);
    eb->evop->add(eb, fd, flags);
    return 0;
}

int event_del(struct event *ev)
{
    list_del(&ev->entry);
    return 0;
}

int event_dispatch(struct event_base *eb, int flags) 
{
    int ret;
    int done = 0;
    const struct eventop *evop = eb->evop;
    while (!done) {
        ret = evop->dispatch(eb, NULL);
        if (ret == -1) {
            return -1;
        }
    }

    return 0;
}


void event_handle(struct event_base *eb, int fd, short flags)
{
    struct event *ev;
    fprintf(stderr, "%s:%d fd = %d\n", __func__, __LINE__, fd);
    if (ev->evcb.cb) {
        ev->evcb.cb(fd, flags, ev->evcb.arg);
    }
}
