#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <limits.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "event.h"

#define EPOLL_MAX_NEVENT	4096

struct epollop {
    int epfd;
    int nevents;
    struct epoll_event *events;
};

struct epoll_event_entry {
    int fd;
    struct epoll_event epev;
    struct event_cbs *evcb;
};
static void *epoll_init()
{
    int fd;
    struct epollop *epop;
    fd = epoll_create(1);
    if (-1 == fd) {
        perror("epoll_create");
        return NULL;
    }
    fprintf(stderr, "%s:%d fd = %d\n", __func__, __LINE__, fd);
    epop = (struct epollop *)calloc(1, sizeof(struct epollop));
    if (!epop) {
        fprintf(stderr, "malloc epollop failed!\n");
        return NULL;
    }
    epop->epfd = fd;
    epop->events = (struct epoll_event *)calloc(EPOLL_MAX_NEVENT, sizeof(struct epoll_event));
    if (NULL == epop->events) {
        fprintf(stderr, "malloc epoll event failed!\n");
        return NULL;
    }
    epop->nevents = EPOLL_MAX_NEVENT;

    return epop;
}


static int epoll_add(struct event_base *e, int fd, struct event_cbs *evcb)
{
    struct epoll_event epev;
    struct epollop *epop = e->base;
    struct epoll_event_entry *ee = (struct epoll_event_entry *)calloc(1, sizeof(struct epoll_event_entry));
    ee->fd = fd;
    ee->epev.events = 0;
    ee->epev.data.ptr = (void *)ee;
    ee->evcb = evcb;
    epev.events = EPOLLIN;//XXX
    epev.data.ptr = (void *)ee;

    if (-1 == epoll_ctl(epop->epfd, EPOLL_CTL_ADD, fd, &epev)) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

static int epoll_del(struct event_base *e, int fd, short events)
{

    return 0;
}

#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX) - 999) / 1000)

static int epoll_dispatch(struct event_base *e, struct timeval *tv)
{
    struct epollop *epop = e->base;
    struct epoll_event *events = epop->events;
    int i, n;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }
    n = epoll_wait(epop->epfd, events, epop->nevents, timeout); 
    if (-1 == n) {
        perror("epoll_wait");
        return -1;
    }
    if (0 == n) {
        fprintf(stderr, "epoll_wait() returned no events\n");
        return -1;
    }
    for (i = 0; i < n; i++) {
        int what = events[i].events;
        struct epoll_event_entry *ee = (struct epoll_event_entry *)events[i].data.ptr;

        if (what & (EPOLLHUP|EPOLLERR)) {
        } else {
            if (what & EPOLLIN)
                ee->evcb->ev_in((void *)ee);
            if (what & EPOLLOUT)
                ee->evcb->ev_out(ee);
            if (what & EPOLLRDHUP)
                ee->evcb->ev_err(ee);
        }
    }
    return 0;
}

const struct event_ops epollops = {
	epoll_init,
	epoll_add,
	epoll_del,
	epoll_dispatch,
};

