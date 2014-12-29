#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "libskt/libskt.h"
#include "libptcp/libptcp.h"
#include "protocol.h"
#include "common.h"
#include "debug.h"

struct ptcp_ctx {
    ptcp_socket_t *ps;
    int fd;
    char src_ip[64];
    uint16_t src_port;
    char dst_ip[64];
    uint16_t dst_port;
};

//#define MAX_ADDR_STRING (64)

static int ptcp_open(struct protocol_ctx *sc, const char *url)
{
    struct ptcp_ctx *c = sc->priv;
    int len;
    char *p;
    char *tag = ":";
    p = strstr(url, tag);
    if (!p) {
        printf("ptcp url is invalid\n");
        return -1;
    }
    len = p - url;
    strncpy(c->src_ip, url, len);
    p += strlen(tag);
    c->src_port = atoi(p);
    
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return -1;
    }
    c->ps = ps;

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(c->src_ip);
    si.sin_port = htons(c->src_port);

    ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));
    printf("url: \"ptcp://%s:%d\"\n", c->src_ip, c->src_port);
    ptcp_listen(ps, 0);


    return 0;
}

static int __ptcp_read(struct protocol_ctx *sc, void *buf, int len)
{
//    struct udp_ctx *c = sc->priv;
//    return ptcp_recv(c->ps, c->dst_ip, c->dst_port, buf, len);
    return 0;
}

static int __ptcp_write(struct protocol_ctx *sc, void *buf, int len)
{
    struct ptcp_ctx *c = sc->priv;
#if 0
    int res;
    int step = 1024;
    int left = len;
    void *p = buf;
#endif
#if 1
    len = ptcp_send(c->ps, buf, len);
    usleep(200 * 1000);
#else
    while (left > 0) {
        if (left < 1024)
            step = left;
        res = ptcp_send(c->ps, p, step);
        if (res == -1) {
            return res;
        }
        p += step;
        left -= step;
        usleep(300 * 1000);
    }
#endif
    return len;
}

static void _ptcp_close(struct protocol_ctx *sc)
{
    struct ptcp_ctx *c = sc->priv;
    return ptcp_close(c->ps);
}

struct protocol mp_ptcp_protocol = {
    .name = "ptcp",
    .open = ptcp_open,
    .read = __ptcp_read,
    .write = __ptcp_write,
    .close = _ptcp_close,
    .priv_size = sizeof(struct ptcp_ctx),
};
