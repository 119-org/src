#ifndef _CALC_CALLEE_H_
#define _CALC_CALLEE_H_

#include "librpc.h"

#ifdef __cplusplus
extern "C" {
#endif

int on_calc(struct rpc_srv *r, void *req, void *rep);

#ifdef __cplusplus
}
#endif
#endif
