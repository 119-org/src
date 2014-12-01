#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>


#include <assert.h>
#include "libptcp.h"

static FILE *g_rfp, *g_sfp;
static long g_flen;

void printf_buf(const char *buf, uint32_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n0x%04x: ", buf[i]);
        printf("%02x ", (buf[i] & 0xFF));
    }
    printf("\n");
}


long filesize(FILE *fd)
{
    long curpos, length;
    curpos = ftell(fd);
    fseek(fd, 0L, SEEK_END);
    length = ftell(fd);
    fseek(fd, curpos, SEEK_SET);
    return length;
}

struct xfer_callback {
    void *(*xfer_server_init)(const char *host, uint16_t port);
    void *(*xfer_client_init)(const char *host, uint16_t port);
    int (*xfer_send)(void *arg, void *buf, size_t len);
    int (*xfer_recv)(void *arg, void *buf, size_t len);
    int (*xfer_close)(void *arg);
    int (*xfer_errno)(void *arg);
};

//=====================tcp=======================
void *tcp_server_init(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    socklen_t len = sizeof(si);

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("bind: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        printf("listen: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }

    int *afd = (int *)calloc(1, sizeof(int));
    *afd = accept(fd, (struct sockaddr *)&si, &len);
    if (*afd == -1) {
        printf("accept: %s\n", strerror(errno));
        return NULL;
    }
    return (void *)afd;
}

void *tcp_client_init(const char *host, uint16_t port)
{
    int *fd = (int *)calloc(1, sizeof(int));
    struct sockaddr_in si;
    *fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == *fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(*fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(*fd);
        return NULL;
    }
    return (void *)fd;
}

int tcp_send(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return send(*fd, buf, len, 0);
}

int tcp_recv(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return recv(*fd, buf, len, 0);
}

int tcp_close(void *arg)
{
    int *fd = (int *)arg;
    return close(*fd);
}

int tcp_errno(void *arg)
{
    return errno;
}

//=================udp=======================
void *udp_server_init(const char *host, uint16_t port)
{
    int *fd = (int *)calloc(1, sizeof(int));
    struct sockaddr_in si;
    *fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == *fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == bind(*fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(*fd);
        return NULL;
    }
    return (void *)fd;
}

void *udp_client_init(const char *host, uint16_t port)
{
    int *fd = (int *)calloc(1, sizeof(int));
    struct sockaddr_in si;
    *fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == *fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(*fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(*fd);
        return NULL;
    }
    return (void *)fd;
}

int udp_send(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return send(*fd, buf, len, 0);
}

int udp_recv(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return recv(*fd, buf, len, 0);
}

int udp_close(void *arg)
{
    int *fd = (int *)arg;
    return close(*fd);
}

int udp_errno(void *arg)
{
    return errno;
}

int ptcp_file_send(char *name, const char *host, uint16_t port)
{
    int len, flen, slen, total;
    char buf[1024] = {0};
    FILE *fp = fopen(name, "r");
    assert(fp);
    g_sfp = fp;
    g_flen = total = flen = filesize(fp);
    struct sockaddr_in si;

    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return -1;
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    if (0 != ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }


    sleep(1);
    while (flen > 0) {
        usleep(100 * 1000);
        len = fread(buf, 1, sizeof(buf), fp);
        if (len == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            ptcp_close(ps);
            fclose(fp);
            return -1;
        }
        slen = ptcp_send(ps, buf, len);
        printf("%s:%d slen=%d, len=%d\n", __func__, __LINE__, slen, len);
        flen -= len;
    }
    sleep(1);
    ptcp_close(ps);
    fclose(fp);
    printf("file %s length is %u\n", name, flen);
    return total;
}

int file_send(char *name, struct xfer_callback *cbs)
{
    int len, flen, slen, total;
    char buf[1024] = {0};
    FILE *fp = fopen(name, "r");
    assert(fp);
    g_sfp = fp;
    total = flen = filesize(fp);
    void *arg = cbs->xfer_client_init("127.0.0.1", 5555);

    sleep(1);
    while (flen > 0) {
        printf("%s:%d xxxx\n", __func__, __LINE__);
        len = fread(buf, 1, sizeof(buf), fp);
        if (len == -1) {
            cbs->xfer_close(arg);
            fclose(fp);
            return -1;
        }
        while (1) {
        slen = cbs->xfer_send(arg, buf, len);
        if (slen <= 0) {
            printf("xfer_send error: %d\n", cbs->xfer_errno(arg));
            usleep(500 *1000);
            continue;
        }
        break;
        }
        //assert(slen==len);
        flen -= len;
    }
    sleep(1);
    cbs->xfer_close(arg);
    fclose(fp);
    printf("file %s length is %u\n", name, flen);
    return total;
}

void *ptcp_server_init(const char *host, uint16_t port)
{
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return NULL;
    }

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);

    ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));
    ptcp_listen(ps, 0);
    return ps;
}

int _ptcp_recv(void *arg, void *buf, size_t len)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    return ptcp_recv(ps, buf, len);
}

void *ptcp_client_init(const char *host, uint16_t port)
{
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return NULL;
    }

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    if (0 != ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }
    return ps;
}

int _ptcp_send(void *arg, void *buf, size_t len)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    return ptcp_send(ps, buf, len);
}

int _ptcp_close(void *arg)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    ptcp_close(ps);
    return 0;
}


int ptcp_errno(void *arg)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    return ptcp_get_error(ps);
}

int ptcp_file_recv(char *name, const char *host, uint16_t port)
{
    int len, flen, rlen;
    char buf[128] = {0};
    FILE *fp = fopen(name, "w");
    assert(fp);
    g_rfp = fp;

    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return -1;
    }

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);

    ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));
    ptcp_listen(ps, 0);

    flen = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        rlen = ptcp_recv(ps, buf, sizeof(buf));
        if (rlen > 0) {
            len = fwrite(buf, 1, rlen, fp);
            assert(len==rlen);
            flen += len;
        } else if (ptcp_is_closed(ps)) {
            printf("ptcp is closed\n");
            return -1;
        } else if (EWOULDBLOCK == ptcp_get_error(ps)){
            //printf("ptcp is error: %d\n", ptcp_get_error(ps));
            usleep(100 * 1000);
            continue;
        }
    }
    return flen;
}

int file_recv(char *name, struct xfer_callback *cbs)
{
    int len, flen, rlen;
    char buf[128] = {0};
    FILE *fp = fopen(name, "w");
    assert(fp);
    g_rfp = fp;
    void *arg = cbs->xfer_server_init("127.0.0.1", 5555);
    assert(arg);

    flen = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        rlen = cbs->xfer_recv(arg, buf, sizeof(buf));
        if (rlen == 0) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            cbs->xfer_close(arg);
            fclose(fp);
            break;
        } else if (rlen == -1) {
            if (EWOULDBLOCK == cbs->xfer_errno(arg)){
                //printf("ptcp is error!\n");
                usleep(100 * 1000);
                continue;
            } else {
                printf("%s:%d xxxx\n", __func__, __LINE__);
                cbs->xfer_close(arg);
                fclose(fp);
                return -1;
            }
        }
        len = fwrite(buf, 1, rlen, fp);
        assert(len==rlen);
        flen += len;
    }
    return flen;
}

static void sigterm_handler(int sig)
{
    exit(0);
}

struct xfer_callback tcp_cbs = {
    tcp_server_init,
    tcp_client_init,
    tcp_send,
    tcp_recv,
    tcp_close,
    tcp_errno
};
struct xfer_callback udp_cbs = {
    udp_server_init,
    udp_client_init,
    udp_send,
    udp_recv,
    udp_close,
    udp_errno
};
struct xfer_callback ptcp_cbs = {
    ptcp_server_init,
    ptcp_client_init,
    _ptcp_send,
    _ptcp_recv,
    _ptcp_close,
    ptcp_errno
};
int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("./test -s / -r filename\n");
        return 0;
    }
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, sigterm_handler);

    struct xfer_callback *cbs;
    cbs = &tcp_cbs;
    cbs = &udp_cbs;
    cbs = &ptcp_cbs;
    if (!strcmp(argv[1], "-s")) {
        file_send(argv[2], cbs);
    }
    if (!strcmp(argv[1], "-r")) {
        file_recv(argv[2], cbs);
    }
    return 0;
}
