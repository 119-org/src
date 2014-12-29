#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libskt/libskt.h"
#include "protocol.h"
#include "common.h"
#include "debug.h"

struct tcp_ctx {
    int fd;
    char src_ip[64];
    uint16_t src_port;
    char dst_ip[64];
    uint16_t dst_port;
};

static int tcp_open(struct protocol_ctx *sc, const char *url)
{
    struct tcp_ctx *c = sc->priv;
    struct skt_addr addr;
    skt_addr_list_t *tmp;
    char str[MAX_ADDR_STRING];
    int len;
    char *p;
    char *tag = ":";
    p = strstr(url, tag);
    if (!p) {
        printf("tcp url is invalid\n");
        return -1;
    }
    len = p - url;//"127.0.0.1:2333"
    printf("url = %s, len = %d\n", url, len);
    strncpy(c->dst_ip, url, len);
    p += strlen(tag);
    c->dst_port = atoi(p);
    
    if (0 == skt_get_local_list(&tmp, 0)) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }
    strcpy(c->src_ip, str);
    struct skt_connection *conn = skt_tcp_connect(str, c->dst_port);
    if (!conn) {
        printf("connect %s:%d failed\n", str, c->dst_port);
        return -1;
    }
    c->fd = conn->fd;

    sc->fd = c->fd;
    skt_get_addr_by_fd(&addr, c->fd);
    c->src_port = addr.port;
//    skt_set_noblk(c->fd, 1);
//    skt_set_reuse(c->fd, 1);
    return 0;
}

static int tcp_read(struct protocol_ctx *sc, void *buf, int len)
{
    struct tcp_ctx *c = sc->priv;
    int ret = skt_recv(c->fd, buf, len);
    return ret;
}

static int tcp_write(struct protocol_ctx *sc, void *buf, int len)
{
    struct tcp_ctx *c = sc->priv;
    return skt_send(c->fd, buf, len);
}

static void tcp_close(struct protocol_ctx *sc)
{
    struct tcp_ctx *c = sc->priv;
    return skt_close(c->fd);
}

struct protocol mp_tcp_protocol = {
    .name = "tcp",
    .open = tcp_open,
    .read = tcp_read,
    .write = tcp_write,
    .close = tcp_close,
    .poll = NULL,
    .handle = NULL,
    .priv_size = sizeof(struct tcp_ctx),
};
