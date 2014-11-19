#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <event2/event.h>

#include "common.h"
#include "protocol.h"
#include "buffer.h"
#include "network_agent.h"

static void on_break_event(int fd, short what, void *arg)
{

}

static void on_udp_read(int fd, short what, void *arg)
{
    int len, ret;
    struct buffer_item *item;
    network_agent_t *na = (network_agent_t *)arg;
    struct buffer_ctx *buf_snk = na->buf_snk;
    int flen = 15000;
    void *buf = calloc(1, flen);
    len = protocol_read(na->pc, buf, flen);
    assert(len <= flen);
    item = buffer_item_new(buf, len);
    ret = buffer_push(buf_snk, item);
}

static void notify_to_break_event_loop(struct network_agent *na)
{
    char notify = '1';
    if (!na)
        return;
    if (write(na->pipe_wfd, &notify, 1) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}


static void *network_agent_loop(void *arg)
{
    struct network_agent *na = (struct network_agent *)arg;
    if (!na)
        return NULL;
    event_base_loop(na->ev_base, 0);
    return NULL;
}


struct network_agent *network_agent_create(char *url, struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk)
{
    network_agent_t *na = NULL;
    int fds[2];

    na = (network_agent_t *)calloc(1, sizeof(network_agent_t));
    if (!na) {
        return NULL;
    }
//    na->pc = protocol_new("udp://127.0.0.1:2333");
    na->pc = protocol_new(url);
    if (!na->pc) {
        return NULL;
    }
    if (-1 == protocol_open(na->pc)) {
        return NULL;
    }
    na->ev_base = event_base_new();
    if (!na->ev_base) {
        return NULL;
    }

    na->buf_src = buf_src;
    na->buf_snk = buf_snk;
    printf("on read fd = %d\n", na->pc->fd);
    na->ev_read = event_new(na->ev_base, na->pc->fd, EV_READ|EV_PERSIST, on_udp_read, na);
    if (!na->ev_read) {
        return NULL;
    }
    if (-1 == event_add(na->ev_read, NULL)) {
        return NULL;
    }
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    na->pipe_rfd = fds[0];
    na->pipe_wfd = fds[1];

    na->ev_close = event_new(na->ev_base, na->pipe_rfd, EV_READ|EV_PERSIST, on_break_event, na->pc);
    if (!na->ev_close) {
        return NULL;
    }
    if (-1 == event_add(na->ev_close, NULL)) {
        return NULL;
    }

    return na;
}

int network_agent_dispatch(struct network_agent *na)
{
    pthread_t tid;
    if (0 != pthread_create(&tid, NULL, network_agent_loop, na)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(na);
        return -1;
    }
    return 0;
}

void network_agent_destroy(struct network_agent *na)
{
    if (!na)
        return;

    if (0 != event_base_loopbreak(na->ev_base)) {
        notify_to_break_event_loop(na);
    }

    event_del(na->ev_read);
    event_del(na->ev_close);
    event_base_free(na->ev_base);
    protocol_close(na->pc);
    protocol_free(na->pc);
    free(na);
}
