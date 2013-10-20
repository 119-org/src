#ifndef _LIBSKT_H_
#define _LIBSKT_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum SKT_ERR_CODE {
    SKT_ERR_PARAMS,

};

typedef struct skt_addr {
    uint32_t ip;
    uint16_t port;
} skt_addr_t;

typedef struct skt_addr_list {
    skt_addr_t addr;
    struct skt_addr_list *next;
} skt_addr_list_t;

typedef struct epop {
    struct epoll_event *events;
    int nevents;
    int epfd;  //for epoll
    int sfd;  //for listen socket
    int (*on_recv)(int fd);
    int (*on_send)(int fd);
} epop_t;

extern int skt_init();
extern void skt_deinit(int fd);

extern int skt_tcp_conn(const char *host, uint16_t port);
extern int skt_tcp_bind_listen(uint16_t port);
extern int skt_udp_bind_listen(uint16_t port);
extern int skt_tcp_srv(uint16_t port);
extern int skt_tcp_cli(const char *host, uint16_t port);

extern int skt_udp_srv(const char *host, uint16_t port);
extern int skt_send(int fd, void *buf, size_t len);
extern int skt_recv(int fd, void *buf, size_t len);

extern int skt_set_noblk(int fd);
extern int skt_clr_noblk(int fd);

extern int skt_domain_to_addr(const char *domain, uint16_t port, skt_addr_list_t **list);
extern int skt_get_addr_by_fd(int fd, skt_addr_t addr);
extern int sock_get_host_list(char ip_list[][64], int list_size);
extern int sock_get_remote_addr(int fd, char *ip);
extern int sock_get_host_list_byname(char ip_list[][64], const char *byname, int list_size);


#ifdef __cplusplus
}
#endif
#endif
