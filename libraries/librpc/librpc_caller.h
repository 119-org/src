#ifndef LIBRPC_CALLER_H
#define LIBRPC_CALLER_H

#include "librpc.h"

#ifdef __cplusplus
extern "C" {
#endif

int rpc_call_hello(struct rpc *r, void *args);

#ifdef __cplusplus
}
#endif
#endif
