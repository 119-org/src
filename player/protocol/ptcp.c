#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libptcp/libptcp.h"
#include "libskt/libskt.h"
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

static int ptcp_open(struct protocol_ctx *sc, const char *url)
{
    struct ptcp_ctx *c = sc->priv;
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
    len = p - url;//"127.0.0.1:2333"
    printf("url = %s, len = %d\n", url, len);
    strncpy(c->dst_ip, url, len);
    p += strlen(tag);
    c->dst_port = atoi(p);
    
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return NULL;
    }
    c->ps = ps;
    c->fd = ptcp_socket_fd(ps);
    sc->fd = c->fd;

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(c->dst_ip);
    si.sin_port = htons(c->dst_port);
    if (0 != ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }
    return 0;
}

static int ptcp_read(struct protocol_ctx *sc, void *buf, int len)
{
    struct ptcp_ctx *c = sc->priv;
    uint32_t ip;
    uint16_t port;
    int res;
    int step = 1024;
    int left = len;
    void *p = buf;
#if 1
    usleep(10 * 1000);
    len = ptcp_recv(c->ps, p, len);
#else
    while (left > 0) {
        if (left < step)
            left = step;
        res = ptcp_recv(c->ps, p, step);
        if (res == -1) {
            //printf("ptcp_recv error: %d\n", ptcp_get_error(c->ps));
            continue;
        }
        p += step;
        left -= step;
    }
#endif
    return len;
}

static int ptcp_write(struct protocol_ctx *sc, void *buf, int len)
{
    struct ptcp_ctx *c = sc->priv;
    return ptcp_send(c->ps, buf, len);
}

static void _ptcp_close(struct protocol_ctx *sc)
{
    struct ptcp_ctx *c = sc->priv;
    return ptcp_close(c->ps);
}

struct protocol ipc_ptcp_protocol = {
    .name = "ptcp",
    .open = ptcp_open,
    .read = ptcp_read,
    .write = ptcp_write,
    .close = _ptcp_close,
    .poll = NULL,
    .handle = NULL,
    .priv_size = sizeof(struct ptcp_ctx),
};
