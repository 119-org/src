#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libskt/libskt.h"
#include "protocol.h"
#include "common.h"
#include "debug.h"

struct udp_ctx {
    int fd;
    char src_ip[64];
    uint16_t src_port;
    char dst_ip[64];
    uint16_t dst_port;
};

static int udp_open(struct protocol_ctx *sc, const char *url)
{
    struct udp_ctx *c = sc->priv;
    struct skt_addr addr;
    skt_addr_list_t *tmp;
    char str[MAX_ADDR_STRING];
    char ip[64];
    int len;
    char *p;
    char *tag = ":";
    p = strstr(url, tag);
    if (!p) {
        printf("udp url is invalid\n");
        return -1;
    }
    len = p - url;
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
    c->fd = skt_udp_bind(c->src_ip, 0, 0);//random port
    if (c->fd == -1) {
        printf("bind %s failed\n", ip);
        return -1;
    }
    skt_get_addr_by_fd(&addr, c->fd);
    c->src_port = addr.port;
    skt_set_noblk(c->fd, 1);
    skt_set_reuse(c->fd, 1);
    printf("url: \"udp://%s:%d?localport=%d\"\n", c->src_ip, c->src_port, c->dst_port);
    return 0;
}

static int udp_read(struct protocol_ctx *sc, void *buf, int len)
{
//    struct udp_ctx *c = sc->priv;
//    return skt_recvfrom(c->fd, c->dst_ip, c->dst_port, buf, len);
    return 0;
}

static int udp_write(struct protocol_ctx *sc, void *buf, int len)
{
    struct udp_ctx *c = sc->priv;
    return skt_sendto(c->fd, c->dst_ip, c->dst_port, buf, len);
}

static void udp_close(struct protocol_ctx *sc)
{
    struct udp_ctx *c = sc->priv;
    return skt_close(c->fd);
}

struct protocol ipc_udp_protocol = {
    .name = "udp",
    .open = udp_open,
    .read = udp_read,
    .write = udp_write,
    .close = udp_close,
    .priv_size = sizeof(struct udp_ctx),
};
