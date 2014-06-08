#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "event.h"

#define SELECT_MAX_FD	1024

struct select_ctx {
    int nfds;		/* Highest fd in fd set */
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
};

static void *select_init()
{
    struct select_ctx *sc;
    sc = (struct select_ctx *)calloc(1, sizeof(struct select_ctx));
    if (!sc) {
        err("malloc select_ctx failed!\n");
        return NULL;
    }
    fd_set *rfds = (fd_set *)calloc(1, sizeof(rfds));
    fd_set *wfds = (fd_set *)calloc(1, sizeof(fd_set));
    fd_set *efds = (fd_set *)calloc(1, sizeof(fd_set));
    if (!rfds || !wfds || !efds) {
        err("malloc fd_set failed!\n");
        return NULL;
    }
    sc->rfds = rfds;
    sc->wfds = wfds;
    sc->efds = efds;

    return sc;
}

static int select_add(struct event_base *eb, struct skt_ev *e)
{
    struct select_ctx *sc = eb->base;

    FD_ZERO(sc->rfds);
    FD_ZERO(sc->wfds);
    FD_ZERO(sc->efds);

    if (sc->nfds < e->evfd) {
        sc->nfds = e->evfd;
    }

    if (e->flags & EVENT_READ)
        FD_SET(e->evfd, sc->rfds);
    if (e->flags & EVENT_WRITE)
        FD_SET(e->evfd, sc->wfds);
    if (e->flags & EVENT_EXCEPT)
        FD_SET(e->evfd, sc->efds);
    return 0;
}

static int select_del(struct event_base *eb, struct skt_ev *e)
{
    struct select_ctx *sc = eb->base;
    if (sc->rfds)
        FD_CLR(e->evfd, sc->rfds);
    if (sc->wfds)
        FD_CLR(e->evfd, sc->wfds);
    if (sc->efds)
        FD_CLR(e->evfd, sc->efds);
    return 0;
}

static int select_dispatch(struct event_base *eb, struct timeval *tv)
{
    int i, n;
    int flags;
    struct select_ctx *sc = eb->base;

    n = select(sc->nfds, sc->rfds, sc->wfds, sc->efds, tv);
    if (-1 == n) {
        err("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        err("select timeout\n");
        return 0;
    }
    for (i = 0; i < sc->nfds; i++) {
        if (FD_ISSET(i, sc->rfds))
            flags |= EVENT_READ;
        if (FD_ISSET(i, sc->wfds))
            flags |= EVENT_WRITE;
        if (FD_ISSET(i, sc->efds))
            flags |= EVENT_EXCEPT;
    }

    return 0;
}

const struct event_ops selectops = {
	select_init,
	select_add,
	select_del,
	select_dispatch,
};
