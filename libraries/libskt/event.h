#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdint.h>
#include <stdlib.h>
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

struct event;
struct event_base;

struct event_cbs {
    void (*ev_in)(void *);
    void (*ev_out)(void *);
    void (*ev_err)(void *);
    void *args;
};

struct event_ops {
    void *(*init)();
    int (*add)(struct event_base *eb, struct event *e);
    int (*del)(struct event_base *eb, struct event *e);
    int (*dispatch)(struct event_base *eb, struct timeval *tv);
};

struct event_base {
    void *base;
    const struct event_ops *evop;
};

struct event {
    struct event_base *evbase;
    int evfd;
    int flags;
    struct event_cbs *evcb;
};

enum event_flags {
    EVENT_TIMEOUT  = 1<<0,
    EVENT_READ     = 1<<1,
    EVENT_WRITE    = 1<<2,
    EVENT_SIGNAL   = 1<<3,
    EVENT_PERSIST  = 1<<4,
    EVENT_ET       = 1<<5,
    EVENT_FINALIZE = 1<<6,
    EVENT_CLOSED   = 1<<7,
    EVENT_ERROR    = 1<<8,
    EVENT_EXCEPT   = 1<<9,
};

int event_init();
struct event *event_create(int fd, int flags, struct event_cbs *evcb, void *args);
void event_destory(struct event *e);
int event_add(struct event *e);
int event_del(struct event *e);
int event_dispatch();

#ifdef __cplusplus
}
#endif
#endif
