#ifndef _NETWORK_AGENT_H_
#define _NETWORK_AGENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <event2/event.h>

#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct network_agent {
    struct protocol_ctx *pc;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int on_read_fd;
    int on_write_fd;
    struct queue_ctx *qin;
    struct queue_ctx *qout;
} network_agent_t;

struct network_agent *network_agent_create(struct queue_ctx *qin, struct queue_ctx *qout);
void network_agent_destroy(struct network_agent *na);


#ifdef __cplusplus
}
#endif
#endif
