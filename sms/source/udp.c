#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libskt/libskt.h"
#include "source.h"
#include "common.h"
#include "debug.h"

struct udp_src_ctx {
    int fd;
    char dst_ip[64];
    uint16_t dst_port;

};

static int udp_open(struct source_ctx *sc, const char *url)
{
    struct udp_src_ctx *c = sc->priv;
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
    c->fd = skt_udp_bind(str, c->dst_port);
    if (c->fd == -1) {
        err("bind %s:%d failed\n", ip, port);
        return -1;
    }
    skt_set_noblk(c->fd, 1);
    return 0;
}

static int udp_read(struct source_ctx *sc, void *buf, int len)
{
    struct udp_src_ctx *c = sc->priv;
    struct frame *f = (struct frame *)buf;
    uint32_t ip;
    uint16_t port;
    len = 614400;
    int ret = skt_recvfrom(c->fd, &ip, &port, f->addr, len);
    f->len = ret;
    dbg("ip = 0x%x port = %d, ret = %d, len = %d\n", ip, port, ret, len);

    return 0;
}
static int udp_write(struct source_ctx *sc, void *buf, int len)
{
    struct udp_src_ctx *c = sc->priv;
}

static void udp_close(struct source_ctx *sc)
{

}


struct source src_udp_module = {
    .name = "udp",
    .open = udp_open,
    .read = udp_read,
    .write = udp_write,
    .close = udp_close,
    .priv_size = sizeof(struct udp_src_ctx),
};
