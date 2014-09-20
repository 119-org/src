#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "librpc.h"
#include "hello_callee.h"
#include "hello.pb.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int on_hello(struct rpc_srv *r, void *q, void *p)
{
    string *reqbuf = static_cast <string *>(q);
    string *repbuf = static_cast <string *>(p);
    hello::request req;
    hello::reply rep;
    if (!req.ParseFromString(*reqbuf)) {
        fprintf(stderr, "parse message failed!\n");
        return -1;
    }
    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;

    rep.set_ret(SUCCESS);
    rep.set_uint32_arg(4321);
    rep.set_string_arg("world");
    if (!rep.SerializeToString(repbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return -1;
    }
    cout << "reply:<<<<<<<<\n" << rep.DebugString() << "<<<<<<<<<<<<<<<\n" << endl;

    return 0;
}
#ifdef __cplusplus
}
#endif
