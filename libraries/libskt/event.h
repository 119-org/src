#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct event {
    void *base;

};

struct eventop {
    void *(*init)();
    int (*add)(struct event *ev, int fd, short events);
    int (*del)(struct event *ev, int fd, short events);
    int (*dispatch)(struct event *ev, struct timeval *tv);
};


#ifdef __cplusplus
}
#endif
#endif
