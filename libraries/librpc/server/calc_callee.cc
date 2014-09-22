#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "librpc.h"
#include "calc_callee.h"
#include "calc.pb.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int on_calc(struct rpc_srv *r, void *q, void *p)
{
    string *reqbuf = static_cast <string *>(q);
    string *repbuf = static_cast <string *>(p);
    calc::request req;
    calc::reply rep;
    int value;
    if (!req.ParseFromString(*reqbuf)) {
        fprintf(stderr, "parse message failed!\n");
        return -1;
    }
//    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;

    switch (req.ops()) {
    case calc::ADD:
        value = req.arg1() + req.arg2();
        break;
    case calc::SUB:
        value = req.arg1() - req.arg2();
        break;
    case calc::MUL:
        value = req.arg1() * req.arg2();
        break;
    case calc::DIV:
        if (req.arg2() == 0)
            break;
        value = req.arg1() / req.arg2();
        break;
    default:
        fprintf(stderr, "can't find cmd!\n");
        break;
    }

    rep.set_ret(SUCCESS);
    rep.set_result(value);
    if (!rep.SerializeToString(repbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return -1;
    }
//    cout << "reply:<<<<<<<<\n" << rep.DebugString() << "<<<<<<<<<<<<<<<\n" << endl;

    return 0;
}
#ifdef __cplusplus
}
#endif
