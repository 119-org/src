#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <event2/event.h>
#include <event2/thread.h>
#include "agent.h"

/*
 * one source -->[agent]--> multi sinks
 *
 */

#define REGISTER_AGENT(x) { \
    extern struct agent mp_##x##_agent; \
    agent_register(&mp_##x##_agent, sizeof(mp_##x##_agent)); }

static struct agent *first_agent = NULL;
static int agt_registered = 0;

static int agent_register(struct agent *agt, int size)
{
    struct agent **d;
    if (size < sizeof(struct agent)) {
        struct agent *temp = (struct agent *)calloc(1, sizeof(struct agent));
        memcpy(temp, agt, size);
        agt = temp;
    }
    d = &first_agent;
    while (*d != NULL) d = &(*d)->next;
    *d = agt;
    agt->next = NULL;
    return 0;
}

int agent_register_all()
{
    if (agt_registered)
        return -1;
    agt_registered = 1;

    REGISTER_AGENT(downstream);
    REGISTER_AGENT(vdecode);
    REGISTER_AGENT(display);

    return 0;
}

struct agent_ctx *agent_ctx_new(const char *name)
{
    struct agent *p;
    struct agent_ctx *sc = (struct agent_ctx *)calloc(1, sizeof(struct agent_ctx));
    if (!sc) {
        printf("malloc agent context failed!\n");
        return NULL;
    }
    for (p = first_agent; p != NULL; p = p->next) {
        if (!strcmp(name, p->name))
            break;
    }
    if (p == NULL) {
        printf("%s protocol is not support!\n", name);
        return NULL;
    }
    printf("[agent] %s module\n", name);

    sc->ops = p;
    sc->priv = calloc(1, p->priv_size);
    if (!sc->priv) {
        printf("malloc agent priv failed!\n");
        return NULL;
    }
    sc->media_info = calloc(1, sizeof(struct media_params));
    return sc;
}

static void on_agent_read(int fd, short what, void *arg)
{
    struct agent_ctx *ctx = (struct agent_ctx *)arg;
    struct yqueue_item *in_item = NULL;
    void *in_data = NULL;
    void *out_data = NULL;
    int in_len = 0;
    int out_len = 0;
    int last = 0;
    if (ctx->yq_src) {
        in_item = yqueue_pop(ctx->yq_src, fd, &last);
        if (!in_item) {
            printf("fd = %d, yqueue_pop empty\n", fd);
            return;
        }
        in_data = in_item->data;
        in_len = in_item->len;
    }
    pthread_mutex_lock(&ctx->lock);
    ctx->ops->on_read(ctx->priv, in_data, in_len, &out_data, &out_len);
    if (out_data) {
        struct yqueue_item *out_item = yqueue_item_new(out_data, out_len);//memory create
        if (-1 == yqueue_push(ctx->yq_snk, out_item)) {
            printf("buffer_push failed!\n");
        }
    }

    if (ctx->yq_src) {
        if (last) {
            //usleep(100000);//FIXME
            //yqueue_item_free(in_item);
        }
    }
    pthread_mutex_unlock(&ctx->lock);
}

static void on_agent_write(int fd, short what, void *arg)
{
    struct agent_ctx *ctx = (struct agent_ctx *)arg;
    ctx->ops->on_write(ctx->priv);
}


struct agent_ctx *agent_create(const char *name, struct yqueue *yq_src, struct yqueue *yq_snk)
{
    struct agent_ctx *ctx = agent_ctx_new(name);
    if (!ctx) {
        return NULL;
    }
    yqueue_add_ref(yq_src);
    int fd;
    pthread_mutex_init(&ctx->lock, NULL);
    ctx->yq_src = yq_src;
    ctx->yq_snk = yq_snk;
    if (ctx->yq_src) {//middle agent
        memcpy(ctx->media_info, yq_src->media_info, sizeof(yq_src->media_info));
    }
    if (-1 == ctx->ops->open(ctx)) {
        return NULL;
    }
    if (ctx->yq_src) {//middle/last agent
        fd = yqueue_get_available_fd(yq_src);
    } else {//first downstream agent
        fd = ctx->rfd;
    }
    if (ctx->yq_snk) {
        memcpy(yq_snk->media_info, ctx->media_info, sizeof(ctx->media_info));
    }

//    if (-1 == evthread_use_pthreads()) {
//        goto failed;
//    }
    ctx->ev_base = event_base_new();
    if (!ctx->ev_base) {
        goto failed;
    }

    printf("name = %s, ctx = %p, fd = %d\n", name, ctx, fd);
    ctx->ev_read = event_new(ctx->ev_base, fd, EV_READ|EV_PERSIST, on_agent_read, ctx);
    if (!ctx->ev_read) {
        goto failed;
    }
    if (-1 == event_add(ctx->ev_read, NULL)) {
        goto failed;
    }

    ctx->ev_write = event_new(ctx->ev_base, ctx->wfd, EV_WRITE|EV_PERSIST, on_agent_write, ctx);
    if (!ctx->ev_write) {
        goto failed;
    }
    if (-1 == event_add(ctx->ev_write, NULL)) {
        goto failed;
    }

    return ctx;

failed:
    if (!ctx) {
        return NULL;
    }
    if (ctx->ev_read) {
        event_del(ctx->ev_read);
    }
    if (ctx->ev_write) {
        event_del(ctx->ev_write);
    }
    if (ctx->ev_base) {
        event_base_free(ctx->ev_base);
    }
    free(ctx);
    return NULL;
}

static void *agent_loop(void *arg)
{
    struct agent_ctx *ctx = (struct agent_ctx *)arg;
    event_base_loop(ctx->ev_base, 0);
    return NULL;
}

int agent_dispatch(struct agent_ctx *ctx, int block)
{
    if (!ctx) {
        return -1;
    }
    if (block) {
        agent_loop(ctx);
    } else {
        pthread_t tid;
        if (0 != pthread_create(&tid, NULL, agent_loop, ctx)) {
            printf("pthread_create falied: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}

void agent_destroy(struct agent_ctx *ctx)
{
    if (!ctx)
        return;

    if (0 != event_base_loopbreak(ctx->ev_base)) {
    }

    event_del(ctx->ev_read);
    event_del(ctx->ev_write);
    event_base_free(ctx->ev_base);
    free(ctx->priv);
    free(ctx);
}
