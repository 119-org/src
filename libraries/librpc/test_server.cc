#include <stdlib.h>
#include "hello_callee.h"
#include "calc_callee.h"

enum req_id {
    HELLO = 0,
    CALC = 1
};

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
