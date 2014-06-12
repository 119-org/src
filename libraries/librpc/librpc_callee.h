#ifndef LIBRPC_CALLEE_H
#define LIBRPC_CALLEE_H

#ifdef __cplusplus
extern "C" {
#endif

int rpc_hello(struct rpc *r, librpc::hello_req *req, librpc::hello_rep *rep);

#ifdef __cplusplus
}
#endif
#endif
