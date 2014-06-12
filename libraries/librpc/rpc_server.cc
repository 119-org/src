#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "librpc.h"
#include "librpc.pb.h"

using namespace std;

typedef struct rpc {

};

struct rpc *rpc_init()
{
    struct rpt *r = (struct rpc *)calloc(1, sizeof(struct rpc));
    if (!r) {
        fprintf(stderr, "malloc rpc failed!\n");
        return NULL;
    }

    return r;
}

int rpc_dispatch(void *p)
{

    return 0;
}
