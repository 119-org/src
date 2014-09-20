#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "librpc.h"
#include "rpc_client.h"
#include "hello_caller.h"
#include "hello.pb.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int rpc_hello(struct rpc *r, void *args)
{
    hello::request req;
    hello::reply rep;
    string wbuf, rbuf;
    req.set_cmd(HELLO);
//    req.set_allocated_cmd(&rrq);
    req.set_uint32_arg(1234);
    req.set_string_arg("hello");

    if (!req.SerializeToString(&wbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return -1;
    }

    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;

    if (rpc_xfer(r, wbuf, &rbuf)) {
        fprintf(stderr, "rpc transfer failed!\n");
        return -1;
    }

    if (false == rep.ParseFromString(rbuf)) {
        fprintf(stderr, "parse from string failed!\n");
        return -1;
    }
    cout << "reply:<<<<<<<<\n" << rep.DebugString() << "<<<<<<<<<<<<<<<\n" << endl;

    return 0;
}
#ifdef __cplusplus
}
#endif
