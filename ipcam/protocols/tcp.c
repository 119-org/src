#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "libskt/libskt.h"
#include "protocol.h"
#include "common.h"
#include "debug.h"

struct tcp_ctx {
    int sfd;
    int fd;
    char src_ip[64];
    uint16_t src_port;
    struct event_base *evbase;
    struct event *evconn;
};


static void on_new_connect(int fd, short what, void *arg)
{
    struct tcp_ctx *tc = (struct tcp_ctx *)arg;
    struct sockaddr_in si;
    socklen_t len = sizeof(si);
    int afd = accept(tc->sfd, (struct sockaddr *)&si, &len);
    if (afd == -1) {
        perror("accept");
        return;
    }
    tc->fd = afd;
}

static void *tcp_thread_event(void *arg)
{
    struct tcp_ctx *tc = (struct tcp_ctx *)arg;
    if (-1 == evthread_use_pthreads()) {
    }
    tc->evbase = event_base_new();
    tc->evconn = event_new(tc->evbase, tc->sfd, EV_READ|EV_PERSIST, on_new_connect, tc);
    
    if (-1 == event_add(tc->evconn, NULL)) {
        printf("event_add failed!\n");
        return NULL;
    }
    event_base_loop(tc->evbase, 0);
    return NULL;
}

static int tcp_open(struct protocol_ctx *pc, const char *url)
{
    struct tcp_ctx *tc = pc->priv;
    pthread_t tid;
    int len;
    char *p;
    char *tag = ":";
    p = strstr(url, tag);
    if (!p) {
        printf("tcp url is invalid\n");
        return -1;
    }
    len = p - url;
    strncpy(tc->src_ip, url, len);
    p += strlen(tag);
    tc->fd = -1;
    tc->src_port = atoi(p);
    tc->sfd = skt_tcp_bind_listen(tc->src_ip, tc->src_port, 0);//random port
    pthread_create(&tid, NULL, tcp_thread_event, tc);
    return 0;
}

static int tcp_read(struct protocol_ctx *pc, void *buf, int len)
{
    return 0;
}

static int tcp_write(struct protocol_ctx *pc, void *buf, int len)
{
    struct tcp_ctx *tc = pc->priv;
    if (-1 == tc->fd) {
        return -1;
    }
    return skt_send(tc->fd, buf, len);
}

static void tcp_close(struct protocol_ctx *pc)
{
    struct tcp_ctx *tc = pc->priv;
    if (0 != event_base_loopbreak(tc->evbase)) {
    }

    event_del(tc->evconn);
    event_base_free(tc->evbase);
    skt_close(tc->fd);
    skt_close(tc->sfd);
}


struct protocol ipc_tcp_protocol = {
    .name = "tcp",
    .open = tcp_open,
    .read = tcp_read,
    .write = tcp_write,
    .close = tcp_close,
    .priv_size = sizeof(struct tcp_ctx),
};
