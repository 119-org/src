#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "librpc.h"
#include "rpc_client.h"
#include "calc_caller.h"
#include "calc.pb.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int rpc_calc(struct rpc *r, void *args)
{
    string wbuf, rbuf;
    calc::request req;
    calc::reply rep;
    struct calc_args *ca = (struct calc_args *)args;
    req.set_id(CALC);
    req.set_ops(static_cast< ::calc::ops >(ca->opcode));
    req.set_arg1(ca->arg1);
    req.set_arg2(ca->arg2);

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
