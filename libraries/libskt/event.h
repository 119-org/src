#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdint.h>
#include <stdlib.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct event_base;

struct eventop {
    void *(*init)();
    int (*add)(struct event_base *eb, int fd, short events, void *ptr, struct eventcb *evcb);
    int (*del)(struct event_base *eb, int fd, short events);
    int (*dispatch)(struct event_base *eb, struct timeval *tv);
};

struct eventcb {
    void (*event_in)(void *);
    void (*event_out)(void *);
    void (*event_err)(void *);
};


struct event_base {
    void *base;
    const struct eventop *evop;
    struct list_head head;
};

struct event_callback {
    void (*cb)(int fd, short flags, void *args);
    void *arg;
    short flags;
};

struct event {
    struct event_base *evbase;
    int evfd;
    struct event_callback evcb;
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

#ifdef __cplusplus
}
#endif
#endif
