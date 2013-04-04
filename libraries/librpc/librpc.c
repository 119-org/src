
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/epoll.h>

#include "libsock.h"
#include "libmempool.h"
#include "liblog.h"
#include "librpc.h"

static struct epoll_event *epoll_event_init(int *fd, int maxfds)
{
    struct epoll_event *events;
    int i = 0;
    if (0 >= maxfds || NULL == fd)
        return NULL;
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * maxfds);
    for (; i < maxfds; i++) {
        events[i].data.fd = fd[i];
        events[i].events = EPOLLIN | EPOLLET;
    }
    return events;
}

int epoll_handler(int epfd, int *fd, int maxfds)
{
    struct epoll_event *events = epoll_event_init(fd, maxfds);
    struct epoll_event *ev = events;
    int i = 0;
    for (; i < maxfds; i++) {
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd[i], ev);
        ev++;
    }
    return 0;
}

int rpc_server_init(uint16_t port)
{
    int fd;
    fd = sock_tcp_server("0.0.0.0", port);
    if (fd == -1) {
        printf("sock tcp server failed!\n");
        return -1;
    }
    if (-1 == sock_tcp_noblock(fd)) {
        printf("sock tcp server failed!\n");
        return -1;
    }
    return fd;
}

int rpc_client_init(const char *ip, uint16_t port)
{
    int fd;
    fd = sock_tcp_connect(ip, port);
    if (fd == -1) {
        printf("connect failed!\n");
        return -1;
    }
    return fd;
}
