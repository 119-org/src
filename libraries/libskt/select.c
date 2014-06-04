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

#define CALLOC(type, ptr) \
    ptr = (type)calloc(1, sizeof(type));
#define MALLOC(type, ptr) \
    ptr = (type)malloc(sizeof(type));

struct selectop {
    int fds;		/* Highest fd in fd set */
    int fdsz;
    fd_set *ev_rfds;
    fd_set *ev_wfds;
    fd_set *ev_efds;
};

static void *select_init()
{
    struct selectop *sop;
    sop = (struct selectop *)calloc(1, sizeof(struct selectop));
    if (!sop) {
        fprintf(stderr, "malloc selectop failed!\n");
        return NULL;
    }
    return sop;
}

static int select_add(struct event_base *e, int fd, short events)
{
    struct selectop *sop = e->base;
    FD_ZERO(sop->ev_rfds);
    FD_ZERO(sop->ev_wfds);
    FD_ZERO(sop->ev_efds);

    FD_SET(fd, sop->ev_rfds);
    FD_SET(fd, sop->ev_wfds);
    FD_SET(fd, sop->ev_efds);
    return 0;
}

static int select_del()
{

}

static int select_dispatch(struct event_base *e)
{
    int n, nfds;
    fd_set rfds, wfds, efds;
    struct selectop *sop = e->base;
    nfds = sop->fds;

    n = select(nfds, sop->ev_rfds, sop->ev_wfds, sop->ev_efds, NULL);

}

const struct event_ops selectops = {
	select_init,
	select_add,
	select_del,
	select_dispatch,
};

