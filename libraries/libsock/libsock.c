
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "libsock.h"

#define LISTEN_MAX_BACKLOG	20

int sock_create(int type)
{
    if (type != SOCK_DGRAM && type != SOCK_STREAM) {
        perror("Only TCP and UDP types are supported!\n");
        return -1;
    }
    return socket(AF_INET, type, 0);
}

int sock_get_host_list(char ip_list[][64], int list_size)
{
    struct ifaddrs *interfaces = 0;
    struct ifaddrs *temp_if = 0;
    struct sockaddr_in *ipv4;
    struct sockaddr_in6 *ipv6;
    int i = 0;
    int fd = sock_create(SOCK_STREAM);
    if (fd < 0) {
        perror("create SOCK_STREAM failed!\n");
        goto err;
    }

    if (getifaddrs(&interfaces) < 0) {
        perror("Cannot get network interface info");
        goto err;
    }

    for (temp_if = interfaces, i = 0; temp_if; temp_if = temp_if->ifa_next) {
        if (temp_if->ifa_flags & IFF_LOOPBACK) {
            continue;
        }
        if (temp_if->ifa_addr != NULL) {
            char ip_str[64] = { 0 };
            switch (temp_if->ifa_addr->sa_family) {
            case AF_INET:
                ipv4 = (struct sockaddr_in *)temp_if->ifa_addr;
                inet_ntop(AF_INET, (void *)&(ipv4->sin_addr), ip_str,
                          sizeof(ip_str));
                break;
            case AF_INET6:
                ipv6 = (struct sockaddr_in6 *)temp_if->ifa_addr;
                inet_ntop(AF_INET6, (void *)&(ipv6->sin6_addr), ip_str,
                          sizeof(ip_str));
                break;
            default:
                break;
            }
            if (i > list_size - 1) {
                printf("ip_list isn't enough, list_size = %d\n", i + 1);
            } else {
                strncpy((ip_list)[i], ip_str, sizeof(ip_str));
                i++;
            }
        }
    }
    freeifaddrs(interfaces);
    return i;
err:
    close(fd);
    return -1;
}

int sock_get_remote_addr(int fd, char *ip)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (ip == NULL) {
        ip = (char *)malloc(64);
        memset(ip, 0, 64);
    }
    if (0 > getpeername(fd, (struct sockaddr *)&(addr.sin_addr), &len)) {
        return -1;
    }
    strcpy(ip, inet_ntoa(addr.sin_addr));
    return 0;
}

int sock_get_local_addr(int fd, char *ip, int port)
{
    struct sockaddr addr;
    struct sockaddr_in *addr_in;
    socklen_t len = sizeof(addr);
    if (ip == NULL) {
        ip = (char *)malloc(64);
        memset(ip, 0, 64);
    }
    if (0 > getsockname(fd, &addr, &len)) {
        perror("getsockname failed!\n");
        return -1;
    }
    if (addr.sa_family != AF_INET) {
        perror("sa_family is not AF_INET!\n");
        return -1;
    }
    addr_in = (struct sockaddr_in *)&addr;

    inet_ntop(AF_INET, (void *)&(addr_in->sin_addr), ip, sizeof(ip));
    port = ntohs(addr_in->sin_port);

    return 0;
}

int sock_get_host_list_byname(char ip_list[][64], const char *byname,
                              int list_size)
{
    struct hostent *host;
    char str[64] = { 0 };
    char **p;
    int i;

    if (NULL == (host = (struct hostent *)gethostbyname(byname))) {
        perror("gethostbyname failed!\n");
        return -1;
    }
    printf("hostname: %s\n", host->h_name);

    for (p = host->h_aliases; *p != NULL; p++) {
        printf("alias: %s\n", *p);
    }
    if (host->h_addrtype == AF_INET || host->h_addrtype == AF_INET6) {
        p = host->h_addr_list;
        for (i = 0; *p != NULL; p++, i++) {
            inet_ntop(host->h_addrtype, *p, str, sizeof(str));
            if (i > list_size - 1) {
                printf("ip_list buf isn't enough, list_size = %d\n", i + 1);
            } else {
                strncpy(ip_list[i], str, sizeof(str));
            }
        }
    }

    return i;
}

int sock_tcp_listen(int port, const char *ip)
{
    char msg[64] = { 0 };
    struct sockaddr_in addr;
    int fd = sock_create(SOCK_STREAM);
    if (fd < 0) {
        perror("create SOCK_STREAM failed!\n");
        goto err;
    }
#ifdef SO_REUSEADDR
    int reuse = 1;
    if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
        perror("set socket reuse address failed!\n");
        goto err;
    }
#endif

    addr.sin_family = AF_INET;
    if (ip == NULL) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        addr.sin_addr.s_addr = inet_addr(ip);
    }
    addr.sin_port = htons(port);

    if (0 > bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        snprintf(msg, sizeof(msg), "bind(%s:%d)", ip, port);
        perror(msg);
        goto err;
    }

    if (0 > listen(fd, LISTEN_MAX_BACKLOG)) {
        perror("listen");
        goto err;
    }

    return fd;
err:
    close(fd);
    return -1;
}

int sock_tcp_connect(const char *ip, int port)
{
    struct sockaddr_in addr;
    int fd = sock_create(SOCK_STREAM);
    if (fd < 0) {
        perror("create SOCK_STREAM failed!\n");
        goto err;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    if (0 > connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("connect");
        goto err;
    }
    return fd;
err:
    close(fd);
    return -1;
}

int sock_tcp_wait_connect(int srv_fd)
{
    struct sockaddr addr;
    socklen_t len = sizeof(addr);
    int fd;
    int send_buf = 64 * 1024;
    socklen_t send_len = sizeof(send_buf);
    fd = accept(srv_fd, &addr, &len);
    if (fd < 0) {
        perror("accept");
        goto err;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    if (0 > setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)send_buf, send_len)) {
        perror("set send buf len failed!\n");
        goto err;
    }
    return fd;
err:
    close(fd);
    return -1;
}
