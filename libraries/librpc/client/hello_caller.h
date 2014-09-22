#ifndef _HELLO_CALLER_H_
#define _HELLO_CALLER_H_

#include "librpc.h"

#ifdef __cplusplus
extern "C" {
#endif

int rpc_hello(struct rpc *r, void *args);

#ifdef __cplusplus
}
#endif
#endif
