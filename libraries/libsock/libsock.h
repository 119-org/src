#ifndef _LIBSOCKEt_H_
#define _LIBSOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int sock_get_host_list(char ip_list[][64], int list_size);
extern int sock_get_local_addr(int fd, char *ip, int port);
extern int sock_get_remote_addr(int fd, char *ip);
extern int sock_get_host_list_byname(char ip_list[][64], const char *byname,
                                         int list_size);

extern int sock_tcp_listen(int port, const char *ip);
extern int sock_tcp_connect(const char *ip, int port);
extern int sock_tcp_wait_connect(int srv_fd);
extern int sock_send(int fd, const void *buf, int len);
extern int sock_recv(int fd, void *buf, int len);

#ifdef __cplusplus
}
#endif
#endif
