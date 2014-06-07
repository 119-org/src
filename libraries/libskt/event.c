#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "event.h"

extern const struct event_ops selectops;
extern const struct event_ops epollops;

static const struct event_ops *eventops[] = {
//    &selectops,
    &epollops,
};

static struct event_base *g_evbase = NULL;

int event_init()
{
    int i;
    struct event_base *eb = (struct event_base *)calloc(1, sizeof(struct event_base));
    if (!eb) {
        err("malloc failed!\n");
        return -1;
    }
    eb->base = NULL;

    for (i = 0; eventops[i] && !eb->base; i++) {
        eb->base = eventops[i]->init();
        eb->evop = eventops[i];
    }
    g_evbase = eb;

    return 0;
}

struct event *event_create(int fd, int flags, struct event_cbs *evcb, void *args)
{
    struct event *e = (struct event *)calloc(1, sizeof(struct event));
    if (!e) {
        err("malloc event failed!\n");
        return NULL;
    }
    e->evbase = g_evbase;
    e->evfd = fd;
    e->flags = flags;
    e->evcb = evcb;
    e->evcb->args = args;

    return e;
}

void event_destroy(struct event *e)
{
    if (!e)
        return;
    if (e->evcb)
        free(e->evcb);
    free(e);
}

int event_add(struct event *e)
{
    struct event_base *eb = g_evbase;
    if (!e || !eb) {
        err("paraments is NULL\n");
        return -1;
    }
    eb->evop->add(eb, e);
    return 0;
}

int event_del(struct event *e)
{
    struct event_base *eb = g_evbase;
    if (!e || !eb) {
        err("paraments is NULL\n");
        return -1;
    }
    eb->evop->del(eb, e);
    return 0;
}

int event_dispatch()
{
    struct event_base *eb = g_evbase;
    const struct event_ops *evop = eb->evop;
    int ret;
    int done = 0;
    while (!done) {
        ret = evop->dispatch(eb, NULL);
        if (ret == -1) {
            err("dispatch failed\n");
//            return -1;
        }
    }

    return 0;
}
