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

typedef void (*timeout_func_t)(union sigval sv);

static void adjust_clock(ptcp_socket_t *ps);

static void notify_clock(union sigval sv)
{
    void *data = (void *)sv.sival_ptr;
    ptcp_socket_t *sock = (ptcp_socket_t *)data;
    printf("Socket %p: Notifying clock\n", sock);
    ptcp_notify_clock(sock);
    adjust_clock(sock);
//    return FALSE;
}

uint32_t add_timer(uint64_t msec, timeout_func_t func, void *data)
{
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    memset(&sev, 0, sizeof(struct sigevent));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = func;
    sev.sigev_value.sival_ptr = data;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("timer_create");
        return -1;
    }

    its.it_value.tv_sec = msec/1000;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        return -1;
    }

    return timerid;
}

int del_timer(uint32_t id)
{
    return timer_delete(id);
}

uint32_t clock_id = 0;
static void adjust_clock(ptcp_socket_t *ps)
{
    uint64_t timeout = 0;

    if (ptcp_get_next_clock(ps, &timeout)) {
        timeout -= get_monotonic_time () / 1000;
        printf("Socket %p: Adjusting clock to %llu ms\n", ps, timeout);
        if (clock_id != 0)
            del_timer(clock_id);
        clock_id = add_timer(timeout, notify_clock, ps);
    } else {
        printf("Socket %p should be destroyed, it's done\n", ps);
    }
}

void on_opened(ptcp_socket_t *p, void *data)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);

}

void on_readable(ptcp_socket_t *p, void *data)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);

}

void on_writable(ptcp_socket_t *p, void *data)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);

}

void on_closed(ptcp_socket_t *p, uint32_t error, void *data)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);

}

ptcp_write_result_t write(ptcp_socket_t *p, const char *buf, uint32_t len, void *data)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);

    return WR_SUCCESS;
}

void recv_msg(int fd)
{
    int ret;
    char buf[1024] = {0};
    ret = recv(fd, buf, sizeof(buf), 0);
    if (ret == -1) {
        printf("%s:%d: errno = %d: %s\n", __func__, __LINE__, errno, strerror(errno));
    }
    printf("\nrecv msg> %s\n", buf);
}

int server(const char *local_ip, uint16_t local_port)
{
    int i;
    int ret;
    int epfd, fd;
    struct in_addr sa;
    struct epoll_event event;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];

    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return -1;
    }

    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event)) {
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
                recv_msg(evbuf[i].data.fd);
            }
        }
    }
}

int client()
{
    ptcp_callbacks_t cbs = {
        NULL, on_opened, on_readable, on_writable, on_closed, write
    };
    ptcp_socket_t *ps = ptcp_create(&cbs);
    if (ps == NULL) {
        printf("error!\n");
    }
    ptcp_notify_mtu(ps, 1496);
    ptcp_connect(ps);
    adjust_clock(ps);
}

int main(int argc, char **argv)
{
    client();
    sleep(10);
    return 0;
}
