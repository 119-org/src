#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "librpc.h"
#include "librpc.pb.h"
#include "librpc_callee.h"
#include "rpc_client.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int rpc_call_hello(struct rpc *r, void *args)
{
    string wbuf, rbuf;
    librpc::hello_req req;
    librpc::hello_rep rep;
    req.set_uint32_arg(1234);
    req.set_string_arg("hello");

    if (!req.SerializeToString(&wbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return -1;
    }

    if (rpc_xfer(r, wbuf, &rbuf)) {
        fprintf(stderr, "rpc transfer failed!\n");
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
