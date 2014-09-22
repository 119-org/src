#ifndef LIBRPC_H
#define LIBRPC_H

#include <stdint.h>
#include <event.h>

#ifdef __cplusplus
extern "C" {
#endif

//rpc client apis
struct rpc {
    uint32_t ip;
    uint16_t port;
};

struct rpc *rpc_new(const char *ip, uint16_t port);
void rpc_free(struct rpc *r);

/**************************************************/

//rpc server apis
struct rpc_srv {
    int fd;
    uint32_t ip;
    uint16_t port;
    struct event_base *evbase;
    struct bufferevent *evbuf;
};
typedef int (*rpc_callee)(struct rpc_srv *r, void *req, void *rep);

struct rpc_handler {
    int id;
    rpc_callee rc;
};

struct rpc_srv *rpc_srv_init(const struct rpc_handler *rh, const char *ip, uint16_t port);
int rpc_srv_set_handler(struct rpc_handler *rh, int size);
int rpc_srv_dispatch(struct rpc_srv *r);

#ifdef __cplusplus
}
#endif
#endif
