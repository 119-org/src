#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <string.h>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

int rpc_xfer(struct rpc *r, string sbuf, string *rbuf);

#ifdef __cplusplus
}
#endif

#endif
