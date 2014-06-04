#include <stdio.h>
#include <stdlib.h>
#include "event.h"

extern const struct event_ops selectops;
extern const struct event_ops epollops;

static const struct event_ops *eventops[] = {
//    &selectops,
    &epollops,
};

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

int event_add(struct event_base *eb, const struct timeval *tv, int fd, void *ptr)
{
    struct event *ev = (struct event *)ptr;
    if (NULL == ev) {
        return -1;
    }
    ev->evbase = eb;
    ev->evfd = fd;

    eb->evop->add(eb, fd, &ev->evcb);
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
    const struct event_ops *evop = eb->evop;
    while (!done) {
        ret = evop->dispatch(eb, NULL);
        if (ret == -1) {
            return -1;
        }
    }

    return 0;
}
