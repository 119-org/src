#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include "libskt.h"

#define EPOLL_MAX_NEVENT	4096

static struct epoll_event *event_list;
static int		   epfd;
static int		   maxevents;

static int epoll_init(struct skt_conn *c);
static int epoll_process(void *args);
static void epoll_deinit(struct skt_conn *c);

static int epoll_init(struct skt_conn *c)
{
    epfd = epoll_create(1);
    if (-1 == epfd) {
        perror("epoll_create");
        return -1;
    }
    event_list = (struct epoll_event *)malloc(EPOLL_MAX_NEVENT);
    if (event_list == NULL) {
        fprintf(stderr, "malloc epoll_event failed!\n");
        return -1;
    }
    maxevents = EPOLL_MAX_NEVENT;

    return 0;
}

static int epoll_process(void *args)
{
#if 0
    int i, events;
    uint32_t revents;
    intptr_t instance;
    struct skt_conn *c;
    events = epoll_wait(epfd, event_list, maxevents, -1);

    if (events == -1) {
        perror("epoll_wait");
        return -1;
    }
    if (events == 0) {
        fprintf(stderr, "epoll_wait() returned no events");
        return -1;
    }

    for (i = 0; i < events; i++) {
#if 0
        c = event_list[i].data.ptr;
        instance = (uintptr_t) c & 1;
        c = (struct skt_conn *) ((uintptr_t) c & (uintptr_t) ~1);
        rev = c->read;
        if (c->fd == -1 || rev->instance != instance) {
            continue;
        }
#endif
        revents = event_list[i].events;

        if (revents & (EPOLLERR | EPOLLHUP)) {
            fprintf(stderr, "epoll_wait() error on fd:%d ev:%04XD\n", c->fd, revents);
        }


        if ((revents & (EPOLLERR|EPOLLHUP))
             && (revents & (EPOLLIN|EPOLLOUT)) == 0)
        {
            /*
             * if the error events were returned without EPOLLIN or EPOLLOUT,
             * then add these flags to handle the events at least in one
             * active handler
             */

            revents |= EPOLLIN|EPOLLOUT;
        }

        if ((revents & EPOLLIN) && rev->active) {

#if (NGX_HAVE_EPOLLRDHUP)
            if (revents & EPOLLRDHUP) {
                rev->pending_eof = 1;
            }
#endif

            if ((flags & NGX_POST_THREAD_EVENTS) && !rev->accept) {
                rev->posted_ready = 1;

            } else {
                rev->ready = 1;
            }

            if (flags & NGX_POST_EVENTS) {
                queue = (ngx_event_t **) (rev->accept ?
                               &ngx_posted_accept_events : &ngx_posted_events);

                ngx_locked_post_event(rev, queue);

            } else {
                rev->handler(rev);
            }
        }

        wev = c->write;

        if ((revents & EPOLLOUT) && wev->active) {

            if (c->fd == -1 || wev->instance != instance) {

                /*
                 * the stale event from a file descriptor
                 * that was just closed in this iteration
                 */

                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                               "epoll: stale event %p", c);
                continue;
            }

            if (flags & NGX_POST_THREAD_EVENTS) {
                wev->posted_ready = 1;

            } else {
                wev->ready = 1;
            }

            if (flags & NGX_POST_EVENTS) {
                ngx_locked_post_event(wev, &ngx_posted_events);

            } else {
                wev->handler(wev);
            }
        }
    }

#endif
    return 0;
}

static void epoll_deinit(struct skt_conn *c)
{

}

