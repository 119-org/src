#ifndef _LIBSKT_H_
#define _LIBSKT_H_

#include <stdint.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netdb.h>
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDR_STRING (65)

//socket structs

enum skt_connect_type {
    SKT_TCP = 0,
    SKT_UDP,
};

typedef struct skt_addr {
    uint32_t ip;
    uint16_t port;
} skt_addr_t;

typedef struct skt_addr_list {
    skt_addr_t addr;
    struct skt_addr_list *next;
} skt_addr_list_t;

typedef struct skt_connection {
    int fd;
    int type;
    struct sockaddr_in si;
    struct skt_addr remote;
} skt_connection_t;

void skt_log_set_cb(void (*cb)(int level, const char* format, va_list vl));

//socket tcp apis
struct skt_connection *skt_tcp_connect(const char *host, uint16_t port);
int skt_tcp_bind_listen(const char *host, uint16_t port, int reuse);
int skt_accept(int fd, uint32_t *ip, uint16_t *port);

//socket udp apis
struct skt_connection *skt_udp_connect(const char *host, uint16_t port);
int skt_udp_bind(const char *host, uint16_t port, int reuse);

//socket common apis
void skt_close(int fd);

int skt_send(int fd, const void *buf, size_t len);
int skt_sendto(int fd, const char *ip, uint16_t port, const void *buf, size_t len);
int skt_recv(int fd, void *buf, size_t len);
int skt_recvfrom(int fd, uint32_t *ip, uint16_t *port, void *buf, size_t len);

uint32_t skt_addr_pton(const char *ip);
int skt_addr_ntop(char *str, uint32_t ip);

int skt_set_noblk(int fd, int enable);
int skt_set_reuse(int fd, int enable);
int skt_set_tcp_keepalive(int fd, int enable);
int skt_set_buflen(int fd, int len);

int skt_get_local_list(struct skt_addr_list **list, int loopback);
int skt_gethostbyname(struct skt_addr_list **list, const char *name);
int skt_getaddrinfo(skt_addr_list_t **list, const char *domain, const char *port);
int skt_get_addr_by_fd(struct skt_addr *addr, int fd);
int skt_get_remote_addr(struct skt_addr *addr, int fd);
#ifdef __cplusplus
}
#endif
#endif
