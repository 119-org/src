#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libskt/libskt.h"
#include "sink.h"
#include "common.h"
#include "debug.h"

struct udp_snk_ctx {
    int fd;
    char dst_ip[64];
    uint16_t dst_port;
};

static int udp_open(struct sink_ctx *sc, const char *url)
{
    struct udp_snk_ctx *c = sc->priv;
    skt_addr_list_t *tmp;
    char str[MAX_ADDR_STRING];
    char ip[64];
    uint16_t port;
    int len;
    char *p;
    char *tag = ":";
    p = strstr(url, tag);
    if (!p) {
        err("udp url is invalid\n");
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
    c->fd = skt_udp_bind(str, 4321);
    if (c->fd == -1) {
        err("bind %s:%d failed\n", ip, port);
        return -1;
    }
    skt_set_noblk(c->fd, 1);
    return 0;
}

static int udp_read(struct sink_ctx *sc, void *buf, int len)
{
    struct udp_snk_ctx *usc = sc->priv;

    return 0;
}

static int udp_write(struct sink_ctx *sc, void *buf, int len)
{
    struct udp_snk_ctx *c = sc->priv;
    struct frame *f = (struct frame *)buf;
    return skt_sendto(c->fd, c->dst_ip, c->dst_port, f->addr, f->len);
}

static void udp_close(struct sink_ctx *sc)
{

}


struct sink snk_udp_module = {
    .name = "udp",
    .open = udp_open,
    .read = udp_read,
    .write = udp_write,
    .close = udp_close,
    .poll = NULL,
    .handle = NULL,
    .priv_size = sizeof(struct udp_snk_ctx),
};
