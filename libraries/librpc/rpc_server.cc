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
#include "librpc_callee.h"

using namespace std;

#define RECV_BUFLEN	4096


struct rpc_handler {
    int cmd;
    rpc_callee *rc;
};

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

static void signal_cb(evutil_socket_t sig, short events, void *arg)
{
    struct event_base *base = (struct event_base *)arg;
    struct timeval delay = { 0, 0 };

    fprintf(stderr, "\nexit\n");
    event_base_loopexit(base, &delay);
}
static void on_accept(struct evconnlistener *listener, evutil_socket_t fd,
            struct sockaddr *sa, int socklen, void *arg)
{
    struct rpc_srv *r = (struct rpc_srv *)arg;
    r->fd = fd;
    fprintf(stderr, "%s xxxx", __func__);

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
    librpc::rpc_rep rep;
    string repbuf;
    fprintf(stderr, "rpc_reply_error\n");

    rep.set_rpcerrno(librpc::ERROR);
    rep.set_rpcstrerr("parse failed");
    if (!rep.SerializeToString(&repbuf)) {
        fprintf(stderr, "serialize reply buffer failed!\n");
        return -1;
    }
    bufferevent_write(r->evbuf, repbuf.data(), repbuf.length());
    return 0;
}

static int rpc_srv_parse(struct rpc_srv *r, const char *buf, int len)
{
    string strbuf(buf, len);
    string repbuf;
    librpc::rpc_req req;
    librpc::rpc_rep rep;

    if (!req.ParseFromString(strbuf)) {
        fprintf(stderr, "parse message failed!\n");
        return rpc_reply_error(r);
    }
    switch (req.cmd()) {
    case librpc::HELLO:
        rpc_hello(r, &req, &rep);
        break;
    default:
        fprintf(stderr, "can't find cmd!\n");
        rpc_reply_error(r);
        break;
    }

    if (!rep.SerializeToString(&repbuf)) {
        fprintf(stderr, "serialize reply buffer failed!\n");
        return -1;
    }

    bufferevent_write(r->evbuf, repbuf.data(), repbuf.length());
    return 0;
}

struct rpc_srv *rpc_srv_init(const char *ip, uint16_t port)
{
    struct rpc_srv *r;
    struct sockaddr_in si;
    struct evconnlistener *listener;
    struct event *signal_event;

    r = (struct rpc_srv *)calloc(1, sizeof(struct rpc));
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

    signal_event = evsignal_new(r->evbase, SIGINT, signal_cb, (void *)r->evbase);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Could not create/add a signal event!\n");
        goto err;
    }
    return r;
err:
    free(r);
    return NULL;
}

int rpc_srv_add(int cmd, rpc_callee *rc)
{
    struct rpc_handler *rh = (struct rpc_handler *)calloc(1, sizeof(struct rpc_handler));
    if (!rh) {
        fprintf(stderr, "malloc rpc handler failed!\n");
        return -1;
    }

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
