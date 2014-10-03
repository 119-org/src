#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include "libskt_event.h"
#include "libskt_log.h"

#define POLL_MAX_FD	1024
#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX) - 999) / 1000)


struct poll_ctx {
    nfds_t nfds;
    int event_count;
    struct pollfd *fds;
};

static void *poll_init()
{
    struct poll_ctx *pc;
    struct pollfd *fds;
    pc = (struct poll_ctx *)calloc(1, sizeof(struct poll_ctx));
    if (!pc) {
        skt_log(LOG_ERR, "malloc poll_ctx failed!\n");
        return NULL;
    }
    fds = (struct pollfd *)calloc(POLL_MAX_FD, sizeof(struct pollfd));
    if (!fds) {
        skt_log(LOG_ERR, "malloc pollfd failed!\n");
        return NULL;
    }
    pc->fds = fds;

    return pc;
}

static int poll_add(struct event_base *eb, struct skt_ev *e)
{
    struct poll_ctx *pc = eb->base;

    pc->fds[0].fd = e->evfd;

    if (e->flags & EVENT_READ)
        pc->fds[0].events |= POLLIN;
    if (e->flags & EVENT_WRITE)
        pc->fds[0].events |= POLLOUT;
    if (e->flags & EVENT_EXCEPT)
        pc->fds[0].events |= POLLERR;

    return 0;
}
static int poll_del(struct event_base *eb, struct skt_ev *e)
{
//    struct poll_ctx *pc = eb->base;

    return 0;
}
static int poll_dispatch(struct event_base *eb, struct timeval *tv)
{
    struct poll_ctx *pc = eb->base;
    int i, n;
    int flags;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }

    n = poll(pc->fds, pc->nfds, timeout);
    if (-1 == n) {
        skt_log(LOG_ERR, "errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        skt_log(LOG_ERR, "poll timeout\n");
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (pc->fds[i].revents & POLLIN)
            flags |= EVENT_READ;
        if (pc->fds[i].revents & POLLOUT)
            flags |= EVENT_WRITE;
        if (pc->fds[i].revents & (POLLERR|POLLHUP|POLLNVAL))
            flags |= EVENT_EXCEPT;
        pc->fds[i].revents = 0;
    }
    return 0;
}

const struct event_ops pollops = {
	poll_init,
	poll_add,
	poll_del,
	poll_dispatch,
};
