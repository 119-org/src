#ifndef _LIBSKT_EVENT_H_
#define _LIBSKT_EVENT_H_

#include <stdint.h>
#include <stdlib.h>
#include "libskt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*blow is event apis */
struct skt_ev_cbs {
    void (*ev_in)(void *);
    void (*ev_out)(void *);
    void (*ev_err)(void *);
    void *args;
};

struct skt_ev {
    int evfd;
    int flags;
    struct skt_ev_cbs *evcb;
};

enum skt_ev_flags {
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

int skt_ev_init();
struct skt_ev *skt_ev_create(int fd, int flags, struct skt_ev_cbs *evcb, void *args);
void skt_ev_destory(struct skt_ev *e);
int skt_ev_add(struct skt_ev *e);
int skt_ev_del(struct skt_ev *e);
int skt_ev_dispatch();


struct skt_ev;
struct event_base;
struct event_ops {
    void *(*init)();
    int (*add)(struct event_base *eb, struct skt_ev *e);
    int (*del)(struct event_base *eb, struct skt_ev *e);
    int (*dispatch)(struct event_base *eb, struct timeval *tv);
};

struct event_base {
    void *base;
    const struct event_ops *evop;
};
#ifdef __cplusplus
}
#endif
#endif
