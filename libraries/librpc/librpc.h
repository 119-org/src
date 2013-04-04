#ifndef _LIBRPC_H_
#define _LIBRPC_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//common
typedef int (*rpc_func_t)(int *, void *);

//caller
extern int rpc_client_init(const char *ip, uint16_t port);
extern int rpc_call(rpc_func_t f, void *arg);
extern int rpc_set_args();

//callee
extern int rpc_server_init(uint16_t port);
extern int rpc_reg();



#ifdef __cplusplus
}
#endif

#endif
