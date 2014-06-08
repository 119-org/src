#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdint.h>
#include <stdlib.h>
#include "libskt.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

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
