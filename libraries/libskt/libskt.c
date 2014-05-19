#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define LISTEN_MAX_BACKLOG	128

int skt_tcp_conn(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;

    if (-1 == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("socket");
        return -1;
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(fd, (struct sockaddr*)&si, sizeof(si))) {
        perror("connect");
        close(fd);
    }

    return fd;
}

int skt_tcp_bind_listen(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    si.sin_family = AF_INET;
    if (host == NULL)
        si.sin_addr.s_addr = INADDR_ANY;
    else
        si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        perror("socket");
        return -1;
    }

    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        perror("bind");
        return -1;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        perror("listen");
        return -1;
    }

    return fd;
}

int skt_udp_bind(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;

    si.sin_family = AF_INET;
    if (host == NULL)
        si.sin_addr.s_addr = INADDR_ANY;
    else
        si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        perror("socket");
        return -1;
    }

    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        perror("bind");
        return -1;
    }
    return fd;
}

#if 0
int skt_accept()
{
    do {
        afd = accept(epop->sfd, (struct sockaddr*)&client, &addrlen);
        if (afd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        } else {
                            perror("accept");
                        }
                    }
                    fprintf(stderr, "new connect is incoming: %s:%d\n", inet_ntoa(client.sin_addr), client.sin_port);

                    if (-1 == fcntl(afd, F_SETFL, fcntl(afd, F_GETFL) | O_NONBLOCK)) {
                        perror("fcntl");
                        break;
                    }

    } while (1);

}
#endif
int skt_get_local_list(skt_addr_list_t **al, int loopback)
{
    struct ifaddrs * ifs = NULL;
    struct ifaddrs * ifa = NULL;
    skt_addr_list_t *ap, *an;

    if (-1 == getifaddrs(&ifs)) {
        perror("getifaddrs");
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
        perror("getpeername");
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
        perror("getsockname");
        return -1;
    }

    addr->ip = inet_ntoa(si.sin_addr);
    addr->port = ntohs(si.sin_port);

    return 0;
}

uint32_t skt_addr_pton(const char *ip)
{
    struct in_addr ia;
    int ret;

    ret = inet_pton(AF_INET, ip, &ia);
    if (ret == -1) {
        perror("inet_pton");
        return -1;
    } else if (ret == 0) {
        fprintf(stderr, "inet_pton not in presentation format\n");
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
        perror("inet_ntop");
        return -1;
    }
    strncpy(str, tmp, INET_ADDRSTRLEN);
    return 0;
}

int skt_getaddrinfo(skt_addr_list_t **al, const char *domain, const char *port)
{
    int ret;
    const char* errstr;
    struct addrinfo hints, *ai_list, *rp;
    skt_addr_list_t *ap, *an;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_CANONNAME;

    ret = getaddrinfo(domain, port, &hints, &ai_list);
    if (ret != 0) {
        errstr = gai_strerror(ret);
        fprintf(stderr, "%s", errstr);
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
        fprintf(stderr, "Could not get ip address of %s!\n", domain);
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
            fprintf(stderr, "Could not get ip address of %s!\n", name);
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
        perror("fcntl");
        return -1;
    }
    if (enable) {
        flag |= O_NONBLOCK;
    } else {
        flag &= ~O_NONBLOCK;
    }
    if (-1 == fcntl(fd, F_SETFL, flag)) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

int skt_set_reuse(int fd, int enable)
{
    int on = !!enable;

#ifdef SO_REUSEPORT
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
        perror("SO_REUSEPORT");
        return -1;
    }
#endif
#ifdef SO_REUSEADDR
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        perror("SO_REUSEADDR");
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
        perror("SO_KEEPALIVE");
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
        perror("SO_RCVBUF");
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
        perror("SO_SNDBUF");
    }
    return 0;
}

int skt_send(int fd, void *buf, size_t len)
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

int skt_recv(int fd, void *buf, size_t len)
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

