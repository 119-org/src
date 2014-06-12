#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "librpc.h"
#include "librpc.pb.h"
#include "librpc_caller.h"
#include "rpc_client.h"

using namespace std;
#ifdef __cplusplus
extern "C" {
#endif


#define RECV_BUFLEN	1460

struct rpc *rpc_new(const char *ip, uint16_t port)
{
    struct rpc *r = (struct rpc *)calloc(1, sizeof(struct rpc));
    if (!r) {
        fprintf(stderr, "malloc rpc failed!\n");
        return NULL;
    }
    r->ip = inet_addr(ip);
    r->port = htons(port);
    return r;
}

void rpc_free(struct rpc *r)
{
    assert(r);
    free(r);
}

int rpc_connect(struct rpc *r)
{
    int fd;
    struct sockaddr_in si;
    struct timeval tv = {2, 0};

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) {
        perror("socket");
        return -1;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = r->ip;
    si.sin_port = r->port;

    if (-1 == connect(fd, (struct sockaddr *)&si, sizeof(si))) {
        perror("connect");
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    return fd;
}

int rpc_send(int fd, const void *buf, size_t len)
{
    ssize_t n;
    uint8_t *p;
    size_t left;
    int err;

    if (buf == NULL || len == 0) {
        fprintf(stderr, "%s paraments invalid!\n", __func__);
        return -1;
    }

    p = (uint8_t *)buf;
    left = len;
    while (left > 0) {
        n = send(fd, p, left, 0);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            fprintf(stderr, "peer has closed\n");
            perror("send");
            break;
        }
        err = errno;
        if (err == EINTR) {
            continue;
        }
        if (err == EAGAIN || err == EWOULDBLOCK) {
            perror("send");
            return EAGAIN;
        }
    }

    return (len - left);
}

int rpc_recv(int fd, void *buf, size_t len)
{
    int n;
    do {
        n = recv(fd, buf, len, 0);
        if (n == -1) {
            perror("recv");
            return -1;
        } else if (n == 0) {
            fprintf(stderr, "peer has closed\n");
            return -1;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN)
            break;
    } while (0);

    return n;
}

int rpc_xfer(struct rpc *r, string sbuf, string *rbuf)
{
    int n, fd;
    char tbuf[RECV_BUFLEN] = {0};

    fd = rpc_connect(r);
    if (-1 == fd) {
        fprintf(stderr, "rpc_connect failed!\n");
        close(fd);
        return -1;
    }
    n = rpc_send(fd, sbuf.data(), sbuf.length());
    if (n == -1) {
        fprintf(stderr, "rpc_send failed!\n");
        close(fd);
        return -1;
    }
    n = rpc_recv(fd, tbuf, RECV_BUFLEN);
    if (n == -1) {
        fprintf(stderr, "rpc_recv failed!\n");
        close(fd);
        return -1;
    }
    close(fd);
    tbuf[n] = '\0';
    rbuf->assign(tbuf, n);

    return 0;
}

#ifdef __cplusplus
}
#endif
