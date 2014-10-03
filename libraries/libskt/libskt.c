#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include "libskt.h"
#include "libskt_log.h"

#define LISTEN_MAX_BACKLOG	128
#define MTU	(1500 - 42 - 200)
#define MAX_RETRY_CNT	10

typedef struct log_str {
    int val;
    char *str;
} log_str_t;

struct log_str log_prefix[] = {
    {LOG_EMERG, "EMERG"},
    {LOG_ALERT, "ALERT"},
    {LOG_CRIT, "CRIT"},
    {LOG_ERR, "ERR"},
    {LOG_WARNING, "WARNING"},
    {LOG_NOTICE, "NOTICE"},
    {LOG_INFO, "INFO"},
    {LOG_DEBUG, "DEBUG"},
    {-1, NULL},
};

void (*skt_log_cb)(int level, const char* format, va_list vl) = NULL;

void skt_log_set(void (*cb)(int level, const char* format, va_list vl))
{
    skt_log_cb = cb;
}

void skt_vfprintf(int level, const char *fmt, va_list vl)
{
    char buf[1024];
    buf[0] = 0;
    snprintf(buf, sizeof(buf), "%s: ", log_prefix[level].str);
    vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, vl);
    fputs(buf, stderr);
}

void skt_vlog(int level, const char *fmt, va_list vl)
{
    if (skt_log_cb) {
        skt_log_cb(level, fmt, vl);
    } else {
        skt_vfprintf(level, fmt, vl);
    }
}

void skt_log(int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    skt_vlog(level, fmt, vl);
    va_end(vl);
}

static struct skt_connection *_skt_connect(int type, const char *host, uint16_t port)
{
    struct skt_connection *sc = (struct skt_connection *)calloc(1, sizeof(struct skt_connection));
    if (sc == NULL) {
        skt_log(LOG_ERR, "malloc failed!\n");
        return NULL;
    }
    sc->type = type;
    sc->fd = socket(AF_INET, sc->type, 0);
    if (-1 == sc->fd) {
        skt_log(LOG_ERR, "socket: %s\n", strerror(errno));
        return NULL;
    }

    sc->si.sin_family = AF_INET;
    sc->si.sin_addr.s_addr = inet_addr(host);
    sc->si.sin_port = htons(port);
    sc->remote.ip = inet_addr(host);
    sc->remote.port = port;

    if (-1 == connect(sc->fd, (struct sockaddr*)&sc->si, sizeof(sc->si))) {
        skt_log(LOG_ERR, "connect: %s\n", strerror(errno));
        close(sc->fd);
        return NULL;
    }
    return sc;
}

struct skt_connection *skt_tcp_connect(const char *host, uint16_t port)
{
    return _skt_connect(SOCK_STREAM, host, port);
}

struct skt_connection *skt_udp_connect(const char *host, uint16_t port)
{
    return _skt_connect(SOCK_DGRAM, host, port);
}

int skt_tcp_bind_listen(const char *host, uint16_t port, int reuse)
{
    int fd;
    struct sockaddr_in si;

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        skt_log(LOG_ERR, "socket: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == skt_set_reuse(fd, reuse)) {
        skt_log(LOG_ERR, "skt_set_reuse failed!\n");
        close(fd);
        return -1;
    }
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        skt_log(LOG_ERR, "bind: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        skt_log(LOG_ERR, "listen: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

int skt_udp_bind(const char *host, uint16_t port, int reuse)
{
    int fd;
    struct sockaddr_in si;

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        skt_log(LOG_ERR, "socket: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == skt_set_reuse(fd, reuse)) {
        skt_log(LOG_ERR, "skt_set_reuse failed!\n");
        close(fd);
        return -1;
    }
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        skt_log(LOG_ERR, "bind: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

int skt_accept(int fd, uint32_t *ip, uint16_t *port)
{
    struct sockaddr_in si;
    socklen_t len = sizeof(si);
    int afd = accept(fd, (struct sockaddr *)&si, &len);
    if (afd == -1) {
        skt_log(LOG_ERR, "accept: %s\n", strerror(errno));
        return -1;
    } else {
        *ip = si.sin_addr.s_addr;
        *port = ntohs(si.sin_port);
    }
    return afd;
}

void skt_close(int fd)
{
    close(fd);
}

int skt_get_local_list(skt_addr_list_t **al, int loopback)
{
    struct ifaddrs * ifs = NULL;
    struct ifaddrs * ifa = NULL;
    skt_addr_list_t *ap, *an;

    if (-1 == getifaddrs(&ifs)) {
        skt_log(LOG_ERR, "getifaddrs: %s\n", strerror(errno));
        return -1;
    }

    ap = NULL;
    *al = NULL;
    for (ifa = ifs; ifa != NULL; ifa = ifa->ifa_next) {

        char saddr[MAX_ADDR_STRING] = "";
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if (!(ifa->ifa_addr))
            continue;
        if (ifa ->ifa_addr->sa_family == AF_INET) {
            if (!inet_ntop(AF_INET, &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr, saddr, INET_ADDRSTRLEN))
                continue;
            if (strstr(saddr,"169.254.") == saddr)
                continue;
            if (!strcmp(saddr,"0.0.0.0"))
                continue;
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            if (!inet_ntop(AF_INET6, &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr, saddr, INET6_ADDRSTRLEN))
                continue;
            if (strstr(saddr,"fe80") == saddr)
                continue;
            if (!strcmp(saddr,"::"))
                continue;
        } else {
            continue;
        }
        if ((ifa->ifa_flags & IFF_LOOPBACK) && !loopback) 
            continue;

        an = (skt_addr_list_t *)calloc(sizeof(skt_addr_list_t), 1);
        an->addr.ip = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr;
        an->addr.port = 0;
        an->next = NULL;
        if (*al == NULL) {
            *al = an;
            ap = *al;
        } else {
            ap->next = an;
            ap = ap->next;
        }
    }
    freeifaddrs(ifs);
    return 0;
}

int skt_get_remote_addr(struct skt_addr *addr, int fd)
{
    struct sockaddr_in si;
    socklen_t len = sizeof(si);

    if (-1 == getpeername(fd, (struct sockaddr *)&(si.sin_addr), &len)) {
        skt_log(LOG_ERR, "getpeername: %s\n", strerror(errno));
        return -1;
    }
    addr->ip = si.sin_addr.s_addr;
    addr->port = ntohs(si.sin_port);
    return 0;
}

int skt_get_addr_by_fd(struct skt_addr *addr, int fd)
{
    struct sockaddr_in si;
    socklen_t len = sizeof(si);
    memset(&si, 0, len);

    if (-1 == getsockname(fd, (struct sockaddr *)&si, &len)) {
        skt_log(LOG_ERR, "getsockname: %s\n", strerror(errno));
        return -1;
    }

    addr->ip = si.sin_addr.s_addr;
    addr->port = ntohs(si.sin_port);

    return 0;
}

uint32_t skt_addr_pton(const char *ip)
{
    struct in_addr ia;
    int ret;

    ret = inet_pton(AF_INET, ip, &ia);
    if (ret == -1) {
        skt_log(LOG_ERR, "inet_pton: %s\n", strerror(errno));
        return -1;
    } else if (ret == 0) {
        skt_log(LOG_ERR, "inet_pton not in presentation format\n");
        return -1;
    }
    return ia.s_addr;
}

int skt_addr_ntop(char *str, uint32_t ip)
{
    struct in_addr ia;
    char tmp[MAX_ADDR_STRING];

    ia.s_addr = ip;
    if (NULL == inet_ntop(AF_INET, &ia, tmp, INET_ADDRSTRLEN)) {
        skt_log(LOG_ERR, "inet_ntop: %s\n", strerror(errno));
        return -1;
    }
    strncpy(str, tmp, INET_ADDRSTRLEN);
    return 0;
}

int skt_getaddrinfo(skt_addr_list_t **al, const char *domain, const char *port)
{
    int ret;
    struct addrinfo hints, *ai_list, *rp;
    skt_addr_list_t *ap, *an;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_CANONNAME;

    ret = getaddrinfo(domain, port, &hints, &ai_list);
    if (ret != 0) {
        skt_log(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    ap = NULL;
    *al = NULL;
    for (rp = ai_list; rp != NULL; rp = rp->ai_next ) {
        an = (skt_addr_list_t *)calloc(sizeof(skt_addr_list_t), 1);
        an->addr.ip = ((struct sockaddr_in *)rp->ai_addr)->sin_addr.s_addr;
        an->addr.port = ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port);
        an->next = NULL;
        if (*al == NULL) {
            *al = an;
            ap = *al;
        } else {
            ap->next = an;
            ap = ap->next;
        }
    }
    if (al == NULL) {
        skt_log(LOG_ERR, "Could not get ip address of %s!\n", domain);
        return -1;
    }
    freeaddrinfo(ai_list);
    return 0;
}

int skt_gethostbyname(skt_addr_list_t **al, const char *name)
{
    skt_addr_list_t *ap, *an;
    struct hostent *host;
    char **p;

    host = gethostbyname(name);
    if (NULL == host) {
        herror("gethostbyname failed!\n");
        return -1;
    }
    if (host->h_addrtype != AF_INET && host->h_addrtype != AF_INET6) {
        herror("addrtype error!\n");
        return -1;
    }

    ap = NULL;
    *al = NULL;
    for (p = host->h_addr_list; *p != NULL; p++) {
        an = (skt_addr_list_t *)calloc(sizeof(skt_addr_list_t), 1);
        an->addr.ip = ((struct in_addr*)(*p))->s_addr;
        an->addr.port = 0;
        an->next = NULL;
        if (*al == NULL) {
            *al = an;
            ap = *al;
        } else {
            ap->next = an;
            ap = ap->next;
        }
        if (al == NULL) {
            skt_log(LOG_ERR, "Could not get ip address of %s!\n", name);
            return -1;
        }
    }
    printf("hostname: %s\n", host->h_name);

    for (p = host->h_aliases; *p != NULL; p++) {
        printf("alias: %s\n", *p);
    }
    return 0;
}

int skt_set_noblk(int fd, int enable)
{
    int flag;
    flag = fcntl(fd, F_GETFL);
    if (flag == -1) {
        skt_log(LOG_ERR, "fcntl: %s\n", strerror(errno));
        return -1;
    }
    if (enable) {
        flag |= O_NONBLOCK;
    } else {
        flag &= ~O_NONBLOCK;
    }
    if (-1 == fcntl(fd, F_SETFL, flag)) {
        skt_log(LOG_ERR, "fcntl: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int skt_set_reuse(int fd, int enable)
{
    int on = !!enable;

#ifdef SO_REUSEPORT
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
        skt_log(LOG_ERR, "setsockopt SO_REUSEPORT: %s\n", strerror(errno));
        return -1;
    }
#endif
#ifdef SO_REUSEADDR
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        skt_log(LOG_ERR, "setsockopt SO_REUSEADDR: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int skt_set_tcp_keepalive(int fd, int enable)
{
    int on = !!enable;

#ifdef SO_KEEPALIVE
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const void*)&on, (socklen_t) sizeof(on))) {
        skt_log(LOG_ERR, "setsockopt SO_KEEPALIVE: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int skt_set_buflen(int fd, int size)
{
    int sz;

    sz = size;
    while (sz > 0) {
        if (-1 == setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const void*) (&sz), (socklen_t) sizeof(sz))) {
            sz = sz / 2;
        } else {
            break;
        }
    }

    if (sz < 1) {
        skt_log(LOG_ERR, "setsockopt SO_RCVBUF: %s\n", strerror(errno));
    }

    sz = size;
    while (sz > 0) {
        if (-1 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const void*) (&sz), (socklen_t) sizeof(sz))) {
            sz = sz / 2;
        } else {
            break;
        }
    }

    if (sz < 1) {
        skt_log(LOG_ERR, "setsockopt SO_SNDBUF: %s\n", strerror(errno));
    }
    return 0;
}

int skt_send(int fd, const void *buf, size_t len)
{
    ssize_t n;
    void *p = (void *)buf;
    size_t left = len;
    size_t step = MTU;
    int cnt = 0;

    if (buf == NULL || len == 0) {
        fprintf(stderr, "%s paraments invalid!\n", __func__);
        return -1;
    }

    while (left > 0) {
        if (left < step)
            step = left;
        n = send(fd, p, step, 0);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            perror("send");
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
//            perror("send");
            cnt++;
            if (cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        return -1;
    }
    return (len - left);
}

int skt_sendto(int fd, const char *ip, uint16_t port, const void *buf, size_t len)
{
    ssize_t n;
    void *p = (void *)buf;
    size_t left = len;
    size_t step = MTU;
    struct sockaddr_in sa;
    int cnt = 0;

    if (buf == NULL || len == 0) {
        fprintf(stderr, "%s paraments invalid!\n", __func__);
        return -1;
    }
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip);
    sa.sin_port = htons(port);

    while (left > 0) {
        if (left < step)
            step = left;
        n = sendto(fd, p, step, 0, (struct sockaddr*)&sa, sizeof(sa));
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            perror("sendto");
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
//            perror("sendto");
            cnt++;
            if (cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        return -1;
    }
    return (len - left);
}

int skt_recv(int fd, void *buf, size_t len)
{
    int n;
    void *p = buf;
    size_t left = len;
    size_t step = MTU;
    int cnt = 0;
    if (buf == NULL || len == 0) {
        fprintf(stderr, "%s paraments invalid!\n", __func__);
        return -1;
    }
    while (left > 0) {
        if (left < step)
            step = left;
        n = recv(fd, p, step, 0);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
//            perror("recv");
            cnt++;
            if (cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        return -1;
    }
    return (len - left);
}

int skt_send_sync_recv(int fd, const void *sbuf, size_t slen, void *rbuf, size_t rlen, int timeout)
{
    skt_send(fd, sbuf, slen);
    skt_set_noblk(fd, 0);
    skt_recv(fd, rbuf, rlen);

    return 0;
}

int skt_recvfrom(int fd, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
    int n;
    void *p = buf;
    int cnt = 0;
    size_t left = len;
    size_t step = MTU;
    struct sockaddr_in si;
    socklen_t si_len = sizeof(si);

    memset(&si, 0, sizeof(si));

    while (left > 0) {
        if (left < step)
            step = left;
        n = recvfrom(fd, p, step, 0, (struct sockaddr *)&si, &si_len);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
//            perror("recvfrom");
            cnt++;
            if (cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        return -1;
    }

    *ip = si.sin_addr.s_addr;
    *port = ntohs(si.sin_port);

    return (len - left);
}
