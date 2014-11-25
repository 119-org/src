#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "libptcp.h"
#define MAX_EPOLL_EVENT 16

#ifndef TRUE
#define TRUE (1 == 1)
#endif

#ifndef FALSE
#define FALSE (0 == 1)
#endif

void printf_buf(const char *buf, uint32_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n0x%04x: ", buf+i);
        printf("%02x ", (buf[i] & 0xFF));
    }
    printf("\n");
}

//=======================================================

void recv_msg(struct my_struct *my)
{
    int len;
    char buf[1024] = {0};

    ptcp_read(my->ps, buf, 0, NULL);
    len = ptcp_recv(my->ps, buf, 1024);
    if (len > 0) {
        printf("ptcp_recv len=%d, buf=%s\n", len, buf);
    } else {
        printf("ptcp_recv len=%d, buf=%s\n", len, buf);
    }
}

int server()
{
    int i, ret, epfd;
    struct epoll_event event;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    const char *host = "127.0.0.1";
    uint16_t port = 4444;
    struct sockaddr_in si;

    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    __ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));

    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return -1;
    }
    struct my_struct *my = (struct my_struct *)calloc(1, sizeof(*my));
    my->ps = ps;
    my->fd = ptcp_get_fd(ps);
    memset(&event, 0, sizeof(event));
    event.data.ptr = my;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, my->fd, &event)) {
        perror("epoll_ctl");
        close(epfd);
        return -1;
    }
    while (1) {
        ret = epoll_wait(epfd, evbuf, MAX_EPOLL_EVENT, -1);
        if (ret == -1) {
            perror("epoll_wait");
            continue;
        }
        for (i = 0; i < ret; i++) {
            if (evbuf[i].data.fd == -1)
                continue;
            if (evbuf[i].events & (EPOLLERR | EPOLLHUP)) {
                perror("epoll error");
            }
            if (evbuf[i].events & EPOLLOUT) {
                perror("epoll out");
            }
            if (evbuf[i].events & EPOLLIN) {
                recv_msg(evbuf[i].data.ptr);
            }
        }
    }
}

static void *cli_send_thread(void *arg)
{
    struct ptcp_socket_t *p = (struct ptcp_socket_t *)arg;
    char buf[10] = {0};
    int i, len;
    sleep(1);
    for (i = 0; i < 1000; i++) {
        usleep(100 * 1000);
        snprintf(buf, sizeof(buf), "client %d", i);
        len = ptcp_send(p, buf, sizeof(buf));
        printf("ptcp_send len=%d, buf=%s\n", len, buf);
    }
    __ptcp_close(p);
}


void cli_recv_msg(struct my_struct *my)
{
    char buf[1024] = {0};
    ptcp_read(my->ps, buf, 0, NULL);
}

int client()
{
    pthread_t tid;
    int ret;
    int epfd;
    struct epoll_event event;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    int i;
    const char *host = "127.0.0.1";
    uint16_t port = 4444;
    struct sockaddr_in si;
    
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    if (0 != __ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }

    struct my_struct *my = (struct my_struct *)calloc(1, sizeof(*my));
    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return -1;
    }
    
    my->ps = ps;
    my->fd = ptcp_get_fd(ps);
    memset(&event, 0, sizeof(event));
    event.data.ptr = my;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, my->fd, &event)) {
        perror("epoll_ctl");
        close(epfd);
        return -1;
    }

    pthread_create(&tid, NULL, cli_send_thread, ps);
    while (1) {
        ret = epoll_wait(epfd, evbuf, MAX_EPOLL_EVENT, -1);
        if (ret == -1) {
            perror("epoll_wait");
            continue;
        }
        for (i = 0; i < ret; i++) {
            if (evbuf[i].data.fd == -1)
                continue;
            if (evbuf[i].events & (EPOLLERR | EPOLLHUP)) {
                perror("epoll error");
            }
            if (evbuf[i].events & EPOLLOUT) {
                perror("epoll out");
            }
            if (evbuf[i].events & EPOLLIN) {
                cli_recv_msg(evbuf[i].data.ptr);
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("./test -c / -s\n");
        return 0;
    }
    if (!strcmp(argv[1], "-c"))
        client();
    if (!strcmp(argv[1], "-s"))
        server();
    return 0;
}
