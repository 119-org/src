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

#define API_EXPORT

#define EPOLL_MAX_NEVENT	4096
#define RECV_BUFLEN		4096
#define LISTEN_MAX_BACKLOG	128

API_EXPORT int skt_init()
{
    return 0;
}

API_EXPORT void skt_deinit(int fd)
{

}

API_EXPORT int skt_domain_to_addr(const char *domain, uint16_t port, skt_addr_list_t **addr_list)
{
    int ret;
    char str_port[8];
    const char* errstring;
    struct addrinfo hints, *ai_list, *rp;
    skt_addr_list_t *addr_ptr, *addr_node;

    if (domain == NULL || port == 0) {
        fprintf(stderr, "remote host addr cannot be NULL or zero!\n");
        return SKT_ERR_PARAMS;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    snprintf(str_port, sizeof(str_port), "%d", port);
    ret = getaddrinfo(domain, str_port, &hints, &ai_list);
    if (ret != 0) {
        errstring = gai_strerror(ret);
        fprintf(stderr, "%s", errstring);
        return -1;
    }

    addr_ptr = NULL;
    *addr_list = NULL;
    for (rp = ai_list; rp != NULL; rp = rp->ai_next ) {
        addr_node = (skt_addr_list_t *)calloc(sizeof(skt_addr_list_t), 1);
        addr_node->addr.ip = rp->ai_addr;
        addr_node->addr.port = port;
        addr_node->next = NULL;

        if (*addr_list == NULL) {
            *addr_list = addr_node;
            addr_ptr = *addr_list;
        } else {
            addr_ptr->next = addr_node;
            addr_ptr = addr_ptr->next;
        }
    }
    if (addr_list == NULL) {
        fprintf(stderr, "Could not get ip address of %s!\n", domain);
        return -1;
    }

    freeaddrinfo(ai_list);

    return 0;
}

API_EXPORT int skt_tcp_conn(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in addr;

    if (host == NULL || port == 0) {
        fprintf(stderr, "remote host addr cannot be NULL or zero!\n");
        return SKT_ERR_PARAMS;
    }

//    fd = socket(AF_UNSPEC, SOCK_STREAM, 0);
    if (-1 == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
        fprintf(stderr, "socket failed!\n");
        perror("socket");
        return -1;
    }

//    addr.sin_family = AF_UNSPEC;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);

    if (-1 == connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        fprintf(stderr, "connect failed!\n");
        perror("connect");
        close(fd);
    }

    return fd;
}

API_EXPORT int skt_tcp_bind_listen(uint16_t port)
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
    ret = getaddrinfo(NULL, str_port, &hints, &result);
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
        if (0 == listen(fd, SOMAXCONN)) {
            break;
        }
        close(fd);
    }
    if (rp == NULL) {
        printf("Could not bind any address!\n");
        close(fd);
        return -1;
    }
//#if SO_NOBLOCK
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
//#endif
    freeaddrinfo(result);
    return fd;
}

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

API_EXPORT int skt_get_addr_by_fd(int fd, skt_addr_t addr)
{
    struct sockaddr_in addr_in;
    socklen_t len = sizeof(addr_in);
    memset(&addr_in, 0, len);

    if (-1 == getsockname(fd, (struct sockaddr *)&addr, &len)) {
        perror("getsockname failed!\n");
        return -1;
    }

    addr.ip = inet_ntoa(addr_in.sin_addr);
    addr.port = ntohs(addr_in.sin_port);

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

int skt_set_noblk(int fd)
{
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

int skt_clr_noblk(int fd)
{
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
    return 0;
}



API_EXPORT int skt_udp_bind_listen(uint16_t port)
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
    hints.ai_socktype = SOCK_DGRAM;  /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    sprintf(str_port, "%u", port);
    ret = getaddrinfo(NULL, str_port, &hints, &result);
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
        if (0 == listen(fd, SOMAXCONN)) {
            break;
        }
        close(fd);
    }
    if (rp == NULL) {
        printf("Could not bind any address!\n");
        close(fd);
        return -1;
    }
//#if SO_NOBLOCK
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
//#endif
    freeaddrinfo(result);
    return fd;
}

int sock_udp_host(const char *ip, uint16_t port)
{
    if (port < 1024) {
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

API_EXPORT int skt_send(int fd, void *buf, size_t len)
{
    ssize_t n;
    uint8_t *p = (uint8_t *)buf;
    size_t left = len;

    if (buf == NULL || len == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }

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
        } else {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
//                perror("send");
//                return -1;
            }
        }
    }

    return (len - left);
}

API_EXPORT int skt_recv(int fd, void *buf, size_t len)
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
API_EXPORT int skt_recv2(int fd, void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    size_t left = len;
    ssize_t n;

    if (buf == NULL || len == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }

    while (left > 0) {
        n = recv(fd, p, left, 0);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            fprintf(stderr, "peer has closed\n");
            perror("recv");
            break;
        } else {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                perror("recv");
                return -1;
            }
        }
    }

    return (len - left);
}

int on_send_msg(int fd)
{
    int n;
    char buf[RECV_BUFLEN] = {0};

    n = skt_send(fd, buf, sizeof(buf));
    if (n == -1) {
        fprintf(stderr, "send failed!\n");
        return -1;
    }
    return 0;
}

int on_recv_msg(int fd)
{
    int n;
    char recvbuf[RECV_BUFLEN] = {0};

    n = skt_recv(fd, recvbuf, sizeof(recvbuf));
    if (n == -1) {
        fprintf(stderr, "recv failed!\n");
        return -1;
    }
    fprintf(stderr, "recv = %s\n", recvbuf);
    return 0;
}

epop_t *epoll_init(int fd)
{
    int epfd;
    struct epoll_event event;
    epop_t *epop;

    if (-1 == (epfd = epoll_create(1))) {
        perror("epoll_create");
        return NULL;
    }

    if (!(epop = (epop_t *)calloc(1, sizeof(struct epop)))) {
        close(epfd);
        return NULL;
    }

    epop->sfd = fd;
    epop->epfd = epfd;
    epop->events = (struct epoll_event *)calloc(EPOLL_MAX_NEVENT, sizeof(struct epoll_event));
    if (epop->events == NULL) {
        free(epop);
        close(epfd);
        return NULL;
    }
    epop->nevents = EPOLL_MAX_NEVENT;

    epop->on_recv = on_recv_msg;
    epop->on_send = on_send_msg;

    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event)) {
        perror("epoll_ctl");
        free(epop);
        close(epfd);
        return NULL;
    }

    return epop;
}

void *epoll_dispatch(void *args)
{
    int i;
    int afd, nfds;
    epop_t *epop = (epop_t *)args;
    struct epoll_event event;
    struct epoll_event *events = epop->events;
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(struct sockaddr_storage);

    while (1) {
        nfds = epoll_wait(epop->epfd, events, epop->nevents, -1);
        if (nfds == -1) {
            perror("epoll_wait");
        }
        for (i = 0; i < nfds; i++) {
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
            } else if (events[i].data.fd == epop->sfd) {
                while (1) {
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
                    event.data.fd = afd;
                    event.events = EPOLLIN | EPOLLET;
                    if (-1 == epoll_ctl(epop->epfd, EPOLL_CTL_ADD, afd, &event)) {
                        perror("epoll_ctl");
                        break;
                    }
                }

            } else if (events[i].events & EPOLLIN) {
                if (-1 == epop->on_recv(events[i].data.fd)) {
                    printf("on_recv failed!\n");
                }

            } else if (events[i].events & EPOLLOUT) {
                printf("%s:%d\n", __func__, __LINE__);
                if (-1 == epop->on_send(events[i].data.fd)) {
                    printf("on_send failed!\n");
                }

            } else {
                printf("%s:%d\n", __func__, __LINE__);
                close(events[i].data.fd);
            }
        }
    }
    return NULL;
}

int skt_tcp_srv_loop()
{
    int loop = 1;

    while (loop) {
        sleep(1);
    }
    return 0;
}

API_EXPORT int skt_tcp_srv(uint16_t port)
{
    int fd;
    pthread_t tid;
    epop_t *epop;

    fd = skt_tcp_bind_listen(port);
    if (fd == -1) {
        fprintf(stderr, "create listen socket failed!\n");
        return -1;
    }

    if (NULL == (epop = epoll_init(fd))) {
        fprintf(stderr, "epoll_init failed!\n");
        return -1;
    }

    if (0 != pthread_create(&tid, NULL, epoll_dispatch, (void *)epop)) {
        fprintf(stderr, "pthread_create failed!\n");
        return -1;
    }
    
    skt_tcp_srv_loop();

}

int skt_tcp_cli(const char *host, uint16_t port)
{
    int fd;
    int n;
    char buf[] = { "hello world" };

    if (-1 == (fd = skt_tcp_conn(host, port))) {
        printf("connect failed!\n");
        return -1;
    }
    while (1) {
        n = skt_send(fd, buf, strlen(buf));
        if (n == -1) {
            printf("skt_send failed!\n");
            return -1;
        }
        sleep(1);
    }
}
