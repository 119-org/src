#include <stdlib.h>
#include "librpc.h"
#include "librpc_callee.h"

int main(int argc, char **argv)
{
    struct rpc_srv *r = rpc_srv_init("127.0.0.1", 1234);
    rpc_srv_add(HELLO, &rpc_hello);
    rpc_srv_dispatch(r);
    return 0;
}
