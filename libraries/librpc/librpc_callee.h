#ifndef LIBRPC_CALLEE_H
#define LIBRPC_CALLEE_H

#include "librpc.h"

#ifdef __cplusplus
extern "C" {
#endif

enum rpc_req_cmd {
    HELLO = 1,
    CALC = 2,
};

int rpc_hello(struct rpc_srv *r, void *req, void *rep);
int rpc_calc(struct rpc_srv *r, void *req, void *rep);

#ifdef __cplusplus
}
#endif
#endif
