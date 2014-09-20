#include <stdlib.h>
#include "librpc.pb.h"
#include "hello_callee.h"
#include "calc_callee.h"

struct rpc_handler rh[] = {
    {HELLO, on_hello},
    {CALC, on_calc},
};

int main(int argc, char **argv)
{
    int size;
    struct rpc_srv *r = rpc_srv_init(rh, "127.0.0.1", 1234);
    size = sizeof(rh)/sizeof(rh[0]);
    rpc_srv_set_handler(rh, size);
    rpc_srv_dispatch(r);
    return 0;
}
