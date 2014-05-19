#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>


#include "libskt.h"

#define IP_LIST		2
#define EPOLL_MAX_NEVENT	4096
#define RECV_BUFLEN		409600

typedef struct skt_conn {
    int fd;

} skt_conn_t;
typedef struct epop {
    struct epoll_event *events;
    int nevents;
    int epfd;  //for epoll
    int sfd;  //for listen socket
    int (*on_recv)(int fd);
    int (*on_send)(int fd);
} epop_t;
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
//    fprintf(stderr, "recv = %s\n", recvbuf);
    return 0;
}


epop_t *epoll_init(skt_conn_t *c)
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

    epop->sfd = c->fd;
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
    event.data.fd = c->fd;
    event.events = EPOLLIN | EPOLLET;
    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, c->fd, &event)) {
        perror("epoll_ctl");
        free(epop);
        close(epfd);
        return NULL;
    }

    return epop;
}

static void *epoll_dispatch(void *args)
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
            continue;
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

int skt_tcp_srv(uint16_t port)
{
    int fd;
    pthread_t tid;
    epop_t *epop;
    skt_conn_t c;

    fd = skt_tcp_bind_listen(&c, port);
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
#if 0
int udp_server_run()
{
    int ret;
    int sfd;
    char host[128] = { 0 };
    char sbuf[] = {"hello world"};
    char rbuf[1024] = { 0 };
    uint16_t port;

    sfd = sock_udp_host("127.0.0.1", 1234);
    if (sfd < 0) {
        printf("udp host error!\n");
        return -1;
    }
    while (1) {
//        ret = sock_udp_recv(sfd, buf, sizeof(buf), host, sizeof(host), &port);
        ret = sock_udp_send_on_recv(sfd, sbuf, sizeof(sbuf), rbuf, sizeof(rbuf), host, sizeof(host), &port);
        if (ret == -1) {
            printf("udp recv error!\n");
            continue;
        }
        printf("udp_send_on_recv ret = %d rbuf = %s, host = %s, port = %u\n", ret, rbuf, host, port);
    }
    return 0;
}
#endif

#if 0
int udp_client_run()
{
    int fd;
    int ret;
    uint16_t port;
    char str[128] = { 0 };
    char buf[] = { "hello world" };

    fd = sock_udp_host("127.0.0.1", 1235);
    if (fd < 0) {
        printf("udp host error!\n");
        return -1;
    }
    while (1) {
        ret = sock_udp_send(fd, buf, strlen(buf), "127.0.0.1", 1234);
        if (ret == -1) {
            printf("udp send failed!\n");
            return -1;
        }
        sleep(1);
    }

}
#endif
void usage()
{
    fprintf(stderr, "./test_libskt -s port\n"
                    "./test_libskt -c ip port\n");

}

void addr_test()
{
    char str_ip[INET_ADDRSTRLEN];
    uint32_t net_ip;
    net_ip = skt_addr_pton("192.168.1.123");
    printf("ip = %d\n", net_ip);
    skt_addr_ntop(str_ip, net_ip);
    printf("ip = %s\n", str_ip);

}

void domain_test()
{
    void *p;
    char str[MAX_ADDR_STRING];
    skt_addr_list_t *tmp;
    if (0 == skt_get_local_list(&tmp, 0)) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }

    if (0 == skt_getaddrinfo(&tmp, "www.sina.com", "3478")) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }
    if (0 == skt_gethostbyname(&tmp, "www.baidu.com")) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }

    do {
        p = tmp;
        if (tmp) tmp = tmp->next;
        if (p) free(p);
    } while (tmp);
}

int main(int argc, char **argv)
{

    if (argc < 2) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s")) {
        skt_tcp_srv(atoi(argv[2]));
    } else if (!strcmp(argv[1], "-c")) {
        skt_tcp_cli(argv[2], atoi(argv[3]));
    }
    if (!strcmp(argv[1], "-t")) {
        addr_test();
    }
    if (!strcmp(argv[1], "-d")) {
        domain_test();
    }
    return 0;
}
