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
    int pipe_rfd;
    int pipe_wfd;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;
} network_agent_t;

struct network_agent *network_agent_create(struct buffer_ctx *qin, struct buffer_ctx *qout);
int network_agent_dispatch(struct network_agent *na);
void network_agent_destroy(struct network_agent *na);


#ifdef __cplusplus
}
#endif
#endif
