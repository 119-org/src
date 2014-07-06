#ifndef _LIBSKT_H_
#define _LIBSKT_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDR_STRING (65)

typedef struct skt_addr {
    uint32_t ip;
    uint16_t port;
} skt_addr_t;

typedef struct skt_addr_list {
    skt_addr_t addr;
    struct skt_addr_list *next;
} skt_addr_list_t;

int skt_tcp_conn(const char *host, uint16_t port);
int skt_tcp_bind_listen(const char *host, uint16_t port);
int skt_accept(int fd, uint32_t *ip, uint16_t *port);
int skt_udp_bind(const char *host, uint16_t port);
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

/*blow is event apis */
struct skt_ev_cbs {
    void (*ev_in)(void *);
    void (*ev_out)(void *);
    void (*ev_err)(void *);
    void *args;
};

struct skt_ev {
    int evfd;
    int flags;
    struct skt_ev_cbs *evcb;
};

enum skt_ev_flags {
    EVENT_TIMEOUT  = 1<<0,
    EVENT_READ     = 1<<1,
    EVENT_WRITE    = 1<<2,
    EVENT_SIGNAL   = 1<<3,
    EVENT_PERSIST  = 1<<4,
    EVENT_ET       = 1<<5,
    EVENT_FINALIZE = 1<<6,
    EVENT_CLOSED   = 1<<7,
    EVENT_ERROR    = 1<<8,
    EVENT_EXCEPT   = 1<<9,
};

int skt_ev_init();
struct skt_ev *skt_ev_create(int fd, int flags, struct skt_ev_cbs *evcb, void *args);
void skt_ev_destory(struct skt_ev *e);
int skt_ev_add(struct skt_ev *e);
int skt_ev_del(struct skt_ev *e);
int skt_ev_dispatch();

#ifdef __cplusplus
}
#endif
#endif
