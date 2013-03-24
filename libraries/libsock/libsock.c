
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
#include <arpa/inet.h>

#include "libsock.h"

#define LISTEN_MAX_BACKLOG	128

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

int sock_tcp_server(const char *ip, uint16_t port)
{
    if (port < 1024) {
        printf("socket port should be 1024 ~ 65535\n");
        return -1;
    }

    int fd, ret;
    char str_port[NI_MAXSERV];
    struct addrinfo *result, *rp, hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    sprintf(str_port, "%u", port);
    ret = getaddrinfo(ip, str_port, &hints, &result);
    if (ret != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            perror("socket failed");
            continue;
        }
#ifdef SO_REUSEADDR
        int reuse = 1;
        if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
            perror("set socket reuse address failed!\n");
            close(fd);
            return -1;
        }
#endif
        if (-1 == bind(fd, rp->ai_addr, rp->ai_addrlen)) {
            perror("bind failed");
            continue;
        }
        if (0 == listen(fd, LISTEN_MAX_BACKLOG)) {
            break;
        }
        close(fd);
    }
    if (rp == NULL) {
        printf("Could not bind any address!\n");
        close(fd);
        return -1;
    }
#if SO_NOBLOCK
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
#endif
    freeaddrinfo(result);
    return fd;
}

int sock_tcp_connect(const char *ip, uint16_t port)
{
    if (ip == NULL || port == 0) {
        printf("ip addr or port is invalid!\n");
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd, ret;
    char str_port[NI_MAXSERV];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_flags = 0;

    sprintf(str_port, "%u", port);
    ret = getaddrinfo(ip, str_port, &hints, &result);
    if (ret != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            perror("socket failed");
            continue;
        }
        if (0 == connect(fd, rp->ai_addr, rp->ai_addrlen))
            break;
        close(fd);
    }
    if (rp == NULL) {
        printf("Could not connect!\n");
        close(fd);
        return -1;
    }

    freeaddrinfo(result);

    return fd;
}

int sock_tcp_accept(int sfd, char *host, size_t host_len, uint16_t *port)
{
    if (sfd < 0) {
        printf("accept sfd error!\n");
        return -1;
    }

    int fd, ret;
    char str_port[NI_MAXSERV];
    struct sockaddr_storage client;
    socklen_t addrlen = sizeof(struct sockaddr_storage);

    fd = accept(sfd, (struct sockaddr*)&client, &addrlen);
    if (fd == -1) {
        perror("accept");
        close(fd);
        return -1;
    }
    if (host == NULL || host_len == 0 || port == NULL) {
        printf("no need to get host info\n");
        return fd;
    }
    client.ss_family = AF_INET; 
    ret = getnameinfo((struct sockaddr*)&client, sizeof(struct sockaddr_storage), host, host_len, str_port, sizeof(str_port), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        printf("getnameinfo: %s\n", gai_strerror(ret));
        return -1;
    }
    *port = atoi(str_port);
#ifdef RDNS 
    void* addrptr;
    struct hostent* he;
    size_t in_addrlen;
    struct sockaddr_storage oldsockaddr;
    socklen_t oldsockaddrlen = sizeof(struct sockaddr_storage);
    if (-1 == getsockname(sfd, (struct sockaddr*)&oldsockaddr, &oldsockaddrlen)) {
        printf("getsockname: %s\n", gai_strerror(-1));
        return -1;
    }
    if (oldsockaddrlen > sizeof(struct sockaddr_storage)) {
        printf("getsockname truncated the struct!\n");
        return -1;
    }
    if (oldsockaddr.ss_family == AF_INET) {
        addrptr = &(((struct sockaddr_in*)&client)->sin_addr);
        in_addrlen = sizeof(struct in_addr);
        *port = ntohs(((struct sockaddr_in*)&client)->sin_port);
    } else if ( oldsockaddr.ss_family == AF_INET6) {
        addrptr = &(((struct sockaddr_in6*)&client)->sin6_addr);
        in_addrlen = sizeof(struct in6_addr);
        *port = ntohs(((struct sockaddr_in6*)&client)->sin6_port);
    }
    he = gethostbyaddr(addrptr, in_addrlen, oldsockaddr.ss_family);
    if (he == NULL) {
        printf("gethostbyaddr: %s\n", gai_strerror(-1));
        return -1;
    }
    strncpy(host, he->h_name, host_len);
#endif
    printf("accept_ip = %s, port = %u\n", host, *port);
    return fd;
}

int sock_udp_server(const char *ip, uint16_t port)
{
    if (port < 1024 || port > 65535) {
        printf("socket port should be 1024 ~ 65535\n");
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd, ret;
    char str_port[NI_MAXSERV];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM;  /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    sprintf(str_port, "%u", port);
    ret = getaddrinfo(ip, str_port, &hints, &result);
    if (ret != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            perror("socket failed");
            continue;
        }
#ifdef SO_REUSEADDR
        int reuse = 1;
        if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
            perror("set socket reuse address failed!\n");
            close(fd);
            return -1;
        }
#endif
        ret = bind(fd, rp->ai_addr, rp->ai_addrlen); 
        if (ret == 0) {
            break;
        }
        close(fd);
    }
    if (rp == NULL) {
        printf("Could not bind any address!\n");
        return -1;
    }
    freeaddrinfo(result);
#if SO_NOBLOCK
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
#endif
    return fd;
}

int sock_tcp_send(int fd, const void *buf, size_t buf_len)
{
    ssize_t bytes;

    if (fd < 0 || buf == NULL || buf_len == 0) {
        printf("ip addr or port is invalid!\n");
        return -1;
    }

    bytes = send(fd, buf, buf_len, 0);
    if (bytes == -1)  {
        printf("send: %s\n", gai_strerror(bytes));
        return -1;
    }
    return bytes;
}

int sock_tcp_recv(int fd, void *buf, size_t buf_len, char *host, size_t host_len, uint16_t *port)
{
    int ret;
    char str_port[NI_MAXSERV];
    ssize_t bytes;
    struct sockaddr_storage client;
    socklen_t stor_addrlen = sizeof(struct sockaddr_storage);

    if (fd < 0 || buf == NULL || buf_len == 0) {
        printf("ip addr or port is invalid!\n");
        return -1;
    }

    memset(buf, 0, buf_len);
    memset(host, 0, host_len);

    //FIXME: can not get port of client info after called more than once
    bytes = recvfrom(fd, buf, buf_len, 0, (struct sockaddr*)&client, &stor_addrlen);
    if (bytes ==  -1) {
        printf("recvform: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (host == NULL || host_len == 0 || port == NULL) {
        printf("no need to show host info\n");
        return bytes;
    }

    client.ss_family = AF_INET;
    ret = getnameinfo((struct sockaddr*)&client, sizeof(struct sockaddr_storage), host, host_len, str_port, sizeof(str_port), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        printf("getnameinfo: %s\n", gai_strerror(ret));
        return -1;
    }
    *port = atoi(str_port);
#ifdef RDNS
    void* addrptr;
    struct hostent* he;
    size_t addrlen;
    struct sockaddr_storage oldsockaddr;
    socklen_t oldsockaddrlen = sizeof(struct sockaddr_storage);
    ret = getsockname(fd, (struct sockaddr*)&oldsockaddr, &oldsockaddrlen);
    if (ret != 0) {
        printf("getsockname: %s\n", gai_strerror(ret));
        return -1;
    }

    if (oldsockaddrlen > sizeof(struct sockaddr_storage)) {
        printf("getsockname truncated the struct!\n");
        return -1;
    }
    if (oldsockaddr.ss_family == AF_INET) {
        addrptr = &(((struct sockaddr_in*)&client)->sin_addr);
        addrlen = sizeof(struct in_addr);
        *port = ntohs(((struct sockaddr_in*)&client)->sin_port);
    } else if (oldsockaddr.ss_family == AF_INET6) {
        addrptr = &(((struct sockaddr_in6*)&client)->sin6_addr);
        addrlen = sizeof(struct in6_addr);
        *port = ntohs(((struct sockaddr_in6*)&client)->sin6_port);
    }
    he = gethostbyaddr(addrptr, addrlen, oldsockaddr.ss_family);
    if (he == NULL) {
        printf("gethostbyaddr: %s\n", gai_strerror(-1));
        return -1;
    }
    strncpy(host, he->h_name, host_len);
#endif
    printf("host = %s, port = %u\n", host, *port);
    return bytes;
}

int sock_udp_send(int fd, const void* buf, size_t buf_len, char* ip, uint16_t port)
{
    if (fd < 0 || buf == NULL || buf_len == 0) {
        printf("udp send paraments is invalid!\n");
        return -1;
    }
    int ret = 0;
    int bytes;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    bytes = sendto(fd, buf, buf_len, 0, (struct sockaddr*)&addr, sizeof(addr));
    if(bytes == -1) {
        printf("sendto: %s\n", gai_strerror(bytes));
        close(fd);
        return -1;
    }
    return bytes;
}

int sock_udp_recv(int fd, void *buf, size_t buf_len, char *host, size_t host_len, uint16_t *port)
{
    int ret;
    char str_port[NI_MAXSERV];
    ssize_t bytes;
    struct sockaddr_storage client;
    socklen_t stor_addrlen = sizeof(struct sockaddr_storage);

    if (fd < 0 || buf == NULL || buf_len == 0) {
        printf("ip addr or port is invalid!\n");
        return -1;
    }

    memset(buf, 0, buf_len);
    memset(host, 0, host_len);

    bytes = recvfrom(fd, buf, buf_len, 0, (struct sockaddr*)&client, &stor_addrlen);
    if (bytes ==  -1) {
        printf("recvfrom: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (host == NULL || host_len == 0 || port == NULL) {
        printf("no need to show host info\n");
        return bytes;
    }
    ret = getnameinfo((struct sockaddr*)&client, sizeof(struct sockaddr_storage), host, host_len, str_port, sizeof(str_port), 0);
    if (ret != 0) {
        printf("getnameinfo: %s\n", gai_strerror(ret));
        return -1;
    }
    *port = atoi(str_port);
#ifdef RDNS
    void* addrptr;
    size_t addrlen;
    struct hostent* he;
    struct sockaddr_storage oldsockaddr;
    socklen_t oldsockaddrlen = sizeof(struct sockaddr_storage);
    ret = getsockname(fd, (struct sockaddr*)&oldsockaddr, &oldsockaddrlen);
    if (ret != 0) {
        printf("getsockname: %s\n", gai_strerror(ret));
        return -1;
    }
    if (oldsockaddrlen > sizeof(struct sockaddr_storage)) {
        printf("getsockname truncated the struct!\n");
        return -1;
    }
    if (oldsockaddr.ss_family == AF_INET) {
        addrptr = &(((struct sockaddr_in*)&client)->sin_addr);
        addrlen = sizeof(struct in_addr);
        *port = ntohs(((struct sockaddr_in*)&client)->sin_port);
    } else if (oldsockaddr.ss_family == AF_INET6) {
        addrptr = &(((struct sockaddr_in6*)&client)->sin6_addr);
        addrlen = sizeof(struct in6_addr);
        *port = ntohs(((struct sockaddr_in6*)&client)->sin6_port);
    }
    he = gethostbyaddr(addrptr, addrlen, oldsockaddr.ss_family);
    if (he == NULL) {
        printf("gethostbyaddr: %s\n", gai_strerror(-1));
        return -1;
    }
    strncpy(host, he->h_name, host_len);
#endif
    printf("host = %s, port = %u\n", host, *port);
    return bytes;
}

int sock_udp_send_on_recv(int fd, const void *send_buf, size_t sbuf_len, void *recv_buf, size_t rbuf_len, char* host, size_t host_len, uint16_t *port)
{
    if (fd < 0 || send_buf == NULL || sbuf_len == 0) {
        printf("send_on_recv paraments is valid\n");
        return -1;
    }
    int ret;
    ssize_t nread, nwrite;
    char str_port[NI_MAXSERV];
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    peer_addr_len = sizeof(struct sockaddr_storage);
    nread = recvfrom(fd, recv_buf, rbuf_len, 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
    if (nread == -1) {
        printf("recvform: %s\n", gai_strerror(nread));
        return -1;
    }

    nwrite = sendto(fd, send_buf, sbuf_len, 0, (struct sockaddr *)&peer_addr, peer_addr_len);
    if (nwrite == -1) {
        printf("sendto: %s\n", gai_strerror(nwrite));
        return -1;
    }
    if (nwrite != sbuf_len) {
        printf("send data len is not equal buf len!\n");
        return -1;
    }
    if (host == NULL || host_len == 0 || port == NULL) {
        printf("no need to get host info\n");
        return nwrite;
    }
    ret = getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, host, host_len, str_port, sizeof(str_port), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret == 0)
        printf("Received %ld bytes from %s:%s\n", (long)nread, host, str_port);
    else
        printf("getnameinfo: %s\n", gai_strerror(ret));
    *port = atoi(str_port);

    return nwrite;
}
