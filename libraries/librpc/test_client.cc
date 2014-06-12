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

int main(int argc, char **argv)
{
    test_hello();

}
