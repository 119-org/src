#include <stdlib.h>
#include "librpc.h"
#include "librpc_caller.h"

int test_hello()
{
    struct rpc *r = rpc_new("127.0.0.1", 1234);
    void *p = NULL;
    rpc_call_hello(r, p);
    rpc_free(r);
    return 0;
}

int test_calc()
{
    struct rpc *r = rpc_new("127.0.0.1", 1234);
    struct calc_args ca;
    ca.arg1 = 1234;
    ca.arg2 = 123;
    ca.opcode = DIV;

    rpc_call_calc(r, &ca);
    rpc_free(r);
    return 0;
}

int main(int argc, char **argv)
{
    test_hello();
    test_calc();

}
