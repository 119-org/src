#ifndef LIBRPC_H
#define LIBRPC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpc {
    uint32_t ip;
    uint16_t port;
};

struct rpc *rpc_new(const char *ip, uint16_t port);
void rpc_free(struct rpc *r);

#ifdef __cplusplus
}
#endif
#endif
