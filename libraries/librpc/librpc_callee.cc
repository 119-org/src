#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "librpc.h"
#include "librpc.pb.h"
#include "librpc_callee.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int rpc_hello(struct rpc_srv *r, void *q, void *p)
{
    string wbuf, rbuf;
    librpc::hello_req *req = static_cast <librpc::hello_req *>(q);
    librpc::hello_rep *rep = static_cast <librpc::hello_rep *>(p);
    fprintf(stderr, "string_arg = %s", req->string_arg().c_str());
    fprintf(stderr, "uint32_arg = %d", req->uint32_arg());
    rep->set_uint32_arg(4321);
    rep->set_string_arg("world");

    if (!rep->SerializeToString(&wbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
