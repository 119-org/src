#ifndef _LIBSOCKEt_H_
#define _LIBSOCKET_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int sock_init();
extern int sock_tcp_server(const char *ip, uint16_t port);
extern int sock_tcp_accept(int fd, char *host, size_t host_len, uint16_t *port);
extern int sock_tcp_connect(const char *ip, uint16_t port);
extern int sock_tcp_send(int fd, const void *buf, size_t buf_len);
extern int sock_tcp_recv(int fd, void *buf, size_t buf_len, char *host, size_t host_len, uint16_t *port);

extern int sock_udp_server(const char *ip, uint16_t port);
extern int sock_udp_send(int fd, const void* buf, size_t buf_len, char* host, uint16_t port);
extern int sock_udp_send_on_recv(int fd, const void *sbuf, size_t sbuf_len, void *rbuf, size_t rbuf_len, char* host, size_t host_len, uint16_t *port);
extern int sock_udp_recv(int fd, void *buf, size_t buf_len, char *host, size_t host_len, uint16_t *port);





extern int sock_get_host_list(char ip_list[][64], int list_size);
extern int sock_get_local_addr(int fd, char *ip, int port);
extern int sock_get_remote_addr(int fd, char *ip);
extern int sock_get_host_list_byname(char ip_list[][64], const char *byname, int list_size);


#ifdef __cplusplus
}
#endif
#endif
