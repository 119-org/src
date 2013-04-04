
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include "librpc.h"

void usage()
{
    printf("Usage: run as client/server\n"
           "./test_librpc -s\n"
           "./test_librpc -c\n");

}

void print_msg(int fd)
{
    int ret;
    char str[128] = { 0 };
    char buf[1024] = { 0 };
    uint16_t port;

    ret = sock_tcp_recv(fd, buf, sizeof(buf), str, sizeof(str), &port);
    printf("recv ip = %s, port = %d, len = %d, buf = %s\n", str, port, ret, buf);

}

void rpc_server()
{
    int sfd, afd, epfd;
    int count;
    uint16_t port = 5000;
    char str[128] = { 0 };
    uint16_t client_port;
    struct epoll_event ev[100];

    sfd = rpc_server_init(port);

    epfd = epoll_create(1);
    epoll_handler(epfd, &sfd, 1);
    while (1) {
        count = epoll_wait(epfd, ev, 1, -1);
printf("count = %d\n", count);
//        while (count--) {
        while (1) {
            afd = sock_tcp_accept(ev[count].data.fd, str, sizeof(str), &client_port);
//            if (ev[count].data.fd == sfd)
                print_msg(afd);
//            else
                printf("asdfasn\n");
//            sock_close(afd);
printf("count = %d\n", count);
        }
    }
}

void rpc_client()
{
    int fd;
    int ret;
    char str[128] = { 0 };
    char buf[] = { "hello world" };
    fd = rpc_client_init("127.0.0.1", 9876);
    while (1) {
        ret = sock_tcp_send(fd, buf, strlen(buf));
        if (ret == -1) {
            printf("connect failed!\n");
            return -1;
        }
        printf("send buf = %s\n", buf);
        sleep(1);
    }

}

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s")) {
        rpc_server();
    } else if (!strcmp(argv[1], "-c")) {
        rpc_client();
    }
    return 0;
}
