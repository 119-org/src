#ifndef LIBRPC_CALLER_H
#define LIBRPC_CALLER_H

#include "librpc.h"

#ifdef __cplusplus
extern "C" {
#endif

//hello rpc api
int rpc_call_hello(struct rpc *r, void *args);

//calc rpc api
enum calc_opcode {
    ADD = 1,
    SUB = 2,
    MUL = 3,
    DIV = 4
};

struct calc_args {
    int32_t arg1;
    int32_t arg2;
    int opcode;
};
int rpc_call_calc(struct rpc *r, void *args);

#ifdef __cplusplus
}
#endif
#endif
