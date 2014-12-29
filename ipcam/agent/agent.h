#ifndef _AGENT_H_
#define _AGENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <event2/event.h>
#include "common.h"
#include "yqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct media_params {
    struct {
        int width;
        int height;
    } video;
    struct {
    } audio;
};

struct agent_ctx {
    int rfd;
    int wfd;
    pthread_mutex_t lock;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_write;
    struct yqueue *yq_src;
    struct yqueue *yq_snk;
    struct agent *ops;
    struct media_params *media_info;
    void *priv;
};

struct agent {
    const char *name;
    int (*open)(struct agent_ctx *c);
    int (*on_read)(void *arg, void *in_data, int in_len, void **out_data, int *out_len);
    int (*on_write)(void *arg);
    void (*close)(struct agent_ctx *c);
    int priv_size;
    struct agent *next;
};

struct agent_ctx *agent_create(const char *name, struct yqueue *yq_src, struct yqueue *yq_snk);
int agent_bind(struct agent_ctx *agt_src, struct agent_ctx *agt_snk);
int agent_dispatch(struct agent_ctx *c, int block);
void agent_destroy(struct agent_ctx *c);
int agent_register_all();

#ifdef __cplusplus
}
#endif
#endif
