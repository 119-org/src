#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <event2/listener.h>

#include "librpc.h"
#include "librpc.pb.h"

using namespace std;

#define RECV_BUFLEN	4096

static struct rpc_handler *g_handle_table = NULL;
static int g_handle_table_size = 0;


static int rpc_srv_parse(struct rpc_srv *r, const char *buf, int len);

static void on_read(struct bufferevent *bev, void *arg)
{
    int n;
    char buf[RECV_BUFLEN] = {0};
    struct rpc_srv *r = (struct rpc_srv *)arg;
    struct evbuffer *input = bufferevent_get_input(bev);
    int len = evbuffer_get_length(input);
    if (len == 0) {
        bufferevent_free(bev);
        return;
    }

    n = bufferevent_read(bev, buf, sizeof(buf)-1);
    buf[n] = '\0';
    rpc_srv_parse(r, buf, n);
}

static void on_write(struct bufferevent *bev, void *arg)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) {
//        bufferevent_free(bev);
    }
}

static void on_event(struct bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF) {
//        fprintf(stderr, "Connection closed.\n");
    } else if (events & BEV_EVENT_ERROR) {
        fprintf(stderr, "Got an error on the connection: %s\n", strerror(errno));
    }
    bufferevent_free(bev);
}

static void on_accept(struct evconnlistener *listener, evutil_socket_t fd,
            struct sockaddr *sa, int socklen, void *arg)
{
    struct rpc_srv *r = (struct rpc_srv *)arg;
    r->fd = fd;

    r->evbuf = bufferevent_socket_new(r->evbase, r->fd, BEV_OPT_CLOSE_ON_FREE);
    if (!r->evbuf) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(r->evbase);
        return;
    }
    bufferevent_setcb(r->evbuf, on_read, on_write, on_event, r);
    bufferevent_enable(r->evbuf, EV_READ | EV_WRITE | EV_PERSIST);
}

int rpc_reply_error(struct rpc_srv *r)
{
    reply rep;
    string repbuf;
    fprintf(stderr, "rpc_reply_error\n");

    rep.set_rpc_errno(ERROR);
    rep.set_rpc_strerr("parse failed");
    if (!rep.SerializeToString(&repbuf)) {
        fprintf(stderr, "serialize reply buffer failed!\n");
        return -1;
    }
    bufferevent_write(r->evbuf, repbuf.data(), repbuf.length());
    return 0;
}

static int rpc_srv_parse(struct rpc_srv *r, const char *buf, int len)
{
    int i;
    string reqbuf(buf, len);
    string repbuf;
    request req;
    reply rep;

    if (!req.ParseFromString(reqbuf)) {
        fprintf(stderr, "parse message failed!\n");
        return rpc_reply_error(r);
    }
    for (i = 0; i < g_handle_table_size; i++) {
        if (req.cmd() == g_handle_table[i].cmd) {
            g_handle_table[i].rc(r, &reqbuf, &repbuf);
            break;
        }
    }

    bufferevent_write(r->evbuf, repbuf.data(), repbuf.length());
    return 0;
}

struct rpc_srv *rpc_srv_init(const struct rpc_handler *rh, const char *ip, uint16_t port)
{
    struct rpc_srv *r;
    struct sockaddr_in si;
    struct evconnlistener *listener;

    r = (struct rpc_srv *)calloc(1, sizeof(struct rpc_srv));
    if (!r) {
        fprintf(stderr, "malloc rpc failed!\n");
        return NULL;
    }

    r->evbase = event_init();
    if (!r->evbase) {
        fprintf(stderr, "event init failed!\n");
        goto err;
    }

    memset(&si, 0, sizeof(si));
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(ip);
    si.sin_port = htons(port);

    listener = evconnlistener_new_bind(r->evbase, on_accept, (void *)r, LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE|LEV_OPT_THREADSAFE, -1, (struct sockaddr*)&si, sizeof(si));
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        goto err;
    }

    return r;

err:
    free(r);
    return NULL;
}

int rpc_srv_set_handler(struct rpc_handler *rh, int size)
{
    g_handle_table = rh;
    g_handle_table_size = size;

    return 0;
}

int rpc_srv_dispatch(struct rpc_srv *r)
{
    if (event_base_loop(r->evbase, 0)) {
        fprintf(stderr, "rpc dispatch failed!\n");
        return -1;
    }
    return 0;
}
