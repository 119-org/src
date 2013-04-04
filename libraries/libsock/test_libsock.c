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


#include "libsock.h"

#define IP_LIST		6

int get_host(int argc, char **argv)
{
    char ip_list[IP_LIST][64] = { 0 };
    int num;
    int i;

    num = sock_get_host_list(ip_list, IP_LIST);
//  num = sock_get_host_list_byname(ip_list, "www.sina.com", 2);
    for (i = 0; i < IP_LIST; i++) {
        printf("ip = %s\n", ip_list[i]);
    }
    printf("list num = %d\n", num);

    return 0;
}

int tcp_server_run()
{
    int ret;
    int sfd, fd, fd2;
    char str[128] = { 0 };
    char buf[1024] = { 0 };
    uint16_t port;

    sfd = sock_tcp_server("127.0.0.1", 1234);
    if (sfd < 0) {
        printf("tcp server error!\n");
        return -1;
    }
    fd = sock_tcp_accept(sfd, str, sizeof(str), &port);
    if (fd < 0) {
        printf("tcp accept error!\n");
        return -1;
    }   
    while (1) {
        memset(str, 0, sizeof(str));
        memset(buf, 0, sizeof(buf));
        port = 0;
        ret = sock_tcp_recv(fd, buf, sizeof(buf), str, sizeof(str), &port);
        printf("recv ip = %s, port = %d, len = %d, buf = %s\n", str, port, ret, buf);
        sleep(1);
        ret = sock_tcp_send(fd, buf, strlen(buf));
        printf("send ip = %s, port = %d, len = %d, buf = %s\n", str, port, ret, buf);
    }

    return 0;
}

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

int tcp_client_run()
{
    int fd;
    int ret;
    uint16_t port;
    char str[128] = { 0 };
    char buf[] = { "hello world" };
    fd = sock_tcp_connect("127.0.0.1", 1234);
    if (fd == -1) {
        printf("connect failed!\n");
        return -1;
    }
    while (1) {
        ret = sock_tcp_send(fd, buf, strlen(buf));
        if (ret == -1) {
            printf("connect failed!\n");
            return -1;
        }
        sleep(1);
    }

}
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

int epoll_init(struct epoll_event ev, int fd, int maxfds)
{
    if (maxfds < 0 || fd == NULL) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }
    int i;
    int efd = epoll_create(1);//arg meanless
    if (efd == -1) {
        perror("epoll_create");
        return -1;
    }
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    if (-1 == epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev)) {
        perror("epoll_ctl");
        return -1;
    }
    return efd;
}

void send_http1(int conn_socket)
{
    char *send_buf =
        "HTTP/1.1 200 OK\r\nServer: Reage webserver\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<!DOCTYPE html><html><head><title>epoll learn</title></head><body><h1>Reage Test111111</h1>Hello World! </body></html>\r\n\r\n";
    write(conn_socket, send_buf, strlen(send_buf));
}

void send_http2(int conn_socket)
{
    char *send_buf =
        "HTTP/1.1 200 OK\r\nServer: Reage webserver\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<!DOCTYPE html><html><head><title>epoll learn</title></head><body><h1>Reage Test22222</h1>Hello World! </body></html>\r\n\r\n";
    write(conn_socket, send_buf, strlen(send_buf));
}

#if 0
int main(int argc, char **argv)
{
//    tcp_client_run();
    udp_client_run();
//    tcp_server_run();
//    udp_server_run();
    return 0;
}
#endif

#if 1
int main(int argc, char *argv[])
{
#define MAX_EVENTS	100
#define MAX_SFD		2
    int sfd, afd, efd;
    struct epoll_event *pev;
    struct epoll_event ev;
    struct sockaddr_in client_addr;
    int i;
    int len;
    int cnt;
    int loop = 1;
    sfd = sock_tcp_server("127.0.0.1", 1234);
    if (-1 == sock_tcp_noblock(sfd)) {
        printf("set non block error!\n");
        return -1;
    }
    pev = (struct epoll_event *)calloc(MAX_EVENTS, sizeof(struct epoll_event));
    efd = epoll_init(ev, sfd, MAX_EVENTS);
    while (loop) {
        cnt = epoll_wait(efd, pev, MAX_EVENTS, -1);
        for (i = 0; i < cnt; i++) {
            if (pev[i].events & EPOLLERR || pev[i].events & EPOLLHUP) {
                fprintf(stderr, "epoll error\n");
                sock_close(pev[i].data.fd);
                continue;
            } else if (sfd == pev[i].data.fd) {//income connect
                while (1) {
                    afd = sock_tcp_accept(pev[i].data.fd, 0, 0, 0);
                    if (afd == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* We have processed all incoming
                               connections. */
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }
                    sock_tcp_noblock(afd);
                    ev.data.fd = afd;
                    ev.events = EPOLLIN | EPOLLET;
                    if (-1 == epoll_ctl(efd, EPOLL_CTL_ADD, afd, &ev)) {//add accept event
                        perror("epoll_ctl");
                        return -1;
                    }
                }
                continue;
            } else if (pev[i].events & EPOLLIN) {//input msg
                char recv_buf[4096] = {0};
                sock_tcp_recv(pev[i].data.fd, recv_buf, sizeof(recv_buf), 0, 0, 0);
                printf("recv buf = %s\n", recv_buf);

            } else if (pev[i].events & EPOLLOUT) {//output msg
                char send_buf[] = {"hello world"};
                sock_tcp_send(pev[i].data.fd, send_buf, sizeof(send_buf));

            } else {
                sock_close(pev[i].data.fd);
            }

       }
    }
    return 0;
}
#endif
