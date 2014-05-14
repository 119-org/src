#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>

#include "libstun.h"

#define MAX_EPOLL_EVENT 16
typedef struct addr_info{
    int fd;
    char ip[64];
    uint16_t port;
} addr_info_t;

static char stun_server_ip[64] = {0};
void usage()
{
    printf("./test -s <stun_srv>\n"
           "./test -s <stun_srv> [-c <local_ip> <local_port>]\n");
}

int udp_send(int fd, const char* ip, uint16_t port, const void* buf, uint32_t len)
{
    int ret = 0;
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip);
    sa.sin_port = htons(port);
    ret = sendto(fd, buf, len, 0, (struct sockaddr*)&sa, sizeof(sa));
    if (ret == -1) {
        close(fd);
    }
    return ret;
}

void *keep_alive(void *args)
{
    int fd = *(int *)args;
    while (1) {
        stun_keep_alive(fd);
        sleep(30);
    }
    return NULL;
}

void *send_msg(void *args)
{
    struct addr_info *tmp = (struct addr_info *)args;
    int fd = tmp->fd;
    char *ip = tmp->ip;
    uint16_t port = tmp->port;
    char buf[1024] = {0};
    int ret;
    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("send msg> ");
        scanf("%s", buf);
        ret = udp_send(fd, ip, port, buf, strlen(buf));
        if (ret == -1) {
            printf("errno = %d: %s\n", errno, strerror(errno));
        }
    }
    return NULL;
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


int test(const char *local_ip, uint16_t local_port)
{
    int i;
    int ret;
    int epfd, fd;
    struct in_addr sa;
    struct epoll_event event;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    StunAddress4 mapped;
    pthread_t tid;
    struct addr_info args;
    char ip[64] = {0};
    uint16_t port;

    if (0 == stun_init(stun_server_ip)) {
        printf("stun init failed!\n");
        return -1;
    }
    fd = stun_socket(local_ip, local_port, &mapped);
    if (fd == -1) {
        printf("stun open socket failed!\n");
        return -1;
    }

    stun_nat_type();

    pthread_create(&tid, NULL, keep_alive, (void *)&fd);
    sa.s_addr = ntohl(mapped.addr);
    printf("mapped ip = %s, port = %d\n", inet_ntoa(sa), mapped.port);
    printf("input peer ip: ");
    scanf("%s", ip);
    printf("input peer port: ");
    scanf("%hd", &port);

    if (-1 == (epfd = epoll_create(1))) {
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
    args.fd = fd;
    args.port = port;
    strcpy(args.ip, ip);
    pthread_create(&tid, NULL, send_msg, (void *)&args);

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

int main(int argc, char **argv)
{
    char *ip = NULL;
    uint16_t port = 0;

    if (argc != 3 && argc != 6) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s")) {
        strcpy(stun_server_ip, argv[2]);
    }
    if (argc == 6) {
        ip = argv[4];
        port = atoi(argv[5]);
    }

    test(ip, port);

    return 0;
}
