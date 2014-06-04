#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdint.h>
#include <stdlib.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct event_base;

struct event_cbs {
    void (*ev_in)(void *);
    void (*ev_out)(void *);
    void (*ev_err)(void *);
};

struct event_ops {
    void *(*init)();
    int (*add)(struct event_base *eb, int fd, struct event_cbs *evcb);
    int (*del)(struct event_base *eb, int fd, short events);
    int (*dispatch)(struct event_base *eb, struct timeval *tv);
};

struct event_base {
    void *base;
    const struct event_ops *evop;
    struct list_head head;
};




struct event {
    struct event_base *evbase;
    int evfd;
    struct event_cbs evcb;
    struct list_head entry;
};

enum event_flags {
    EV_TIMEOUT  = 1<<0,
    EV_READ     = 1<<1,
    EV_WRITE    = 1<<2,
    EV_SIGNAL   = 1<<3,
    EV_PERSIST  = 1<<4,
    EV_ET       = 1<<5,
    EV_FINALIZE = 1<<6,
    EV_CLOSED   = 1<<7,
};

struct event_base *event_init();
int event_add(struct event_base *eb, const struct timeval *tv, int fd, void *ptr);
int event_del(struct event *ev);
int event_dispatch(struct event_base *eb, int flags);

#ifdef __cplusplus
}
#endif
#endif
