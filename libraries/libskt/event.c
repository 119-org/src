#include <stdio.h>
#include <stdlib.h>
#include "libskt.h"
#include "libskt_event.h"
#include "libskt_log.h"

extern const struct event_ops selectops;
extern const struct event_ops pollops;
extern const struct event_ops epollops;

static const struct event_ops *eventops[] = {
//    &selectops,
    &epollops,
};

static struct event_base *g_evbase = NULL;

int skt_ev_init()
{
    int i;
    struct event_base *eb = (struct event_base *)calloc(1, sizeof(struct event_base));
    if (!eb) {
        skt_log(LOG_ERR, "malloc failed!\n");
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

struct skt_ev *skt_ev_create(int fd, int flags, struct skt_ev_cbs *evcb, void *args)
{
    struct skt_ev *e = (struct skt_ev *)calloc(1, sizeof(struct skt_ev));
    if (!e) {
        skt_log(LOG_ERR, "malloc skt_ev failed!\n");
        return NULL;
    }
    e->evfd = fd;
    e->flags = flags;
    e->evcb = evcb;
    e->evcb->args = args;

    return e;
}

void skt_ev_destroy(struct skt_ev *e)
{
    if (!e)
        return;
    if (e->evcb)
        free(e->evcb);
    free(e);
}

int skt_ev_add(struct skt_ev *e)
{
    struct event_base *eb = g_evbase;
    if (!e || !eb) {
        skt_log(LOG_ERR, "paraments is NULL\n");
        return -1;
    }
    eb->evop->add(eb, e);
    return 0;
}

int skt_ev_del(struct skt_ev *e)
{
    struct event_base *eb = g_evbase;
    if (!e || !eb) {
        skt_log(LOG_ERR, "paraments is NULL\n");
        return -1;
    }
    eb->evop->del(eb, e);
    return 0;
}

int skt_ev_dispatch()
{
    struct event_base *eb = g_evbase;
    const struct event_ops *evop = eb->evop;
    int ret;
    int done = 0;
    while (!done) {
        ret = evop->dispatch(eb, NULL);
        if (ret == -1) {
            skt_log(LOG_ERR, "dispatch failed\n");
//            return -1;
        }
    }

    return 0;
}
