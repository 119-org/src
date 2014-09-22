#ifndef _CALC_CALLER_H_
#define _CALC_CALLER_H_

#include "librpc.h"

#ifdef __cplusplus
extern "C" {
#endif

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
int rpc_calc(struct rpc *r, void *args);


#ifdef __cplusplus
}
#endif
#endif
