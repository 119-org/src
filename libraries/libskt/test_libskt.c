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

int main(int argc, char **argv)
{
    int fd;
    skt_addr_list_t *p, *addr_list;
    struct sockaddr_in *sin;
    char buf[INET_ADDRSTRLEN];
    const char *str_ip;

    if (argc < 2) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s")) {
        skt_tcp_srv(atoi(argv[2]));
    } else if (!strcmp(argv[1], "-c")) {
        skt_tcp_cli(argv[2], atoi(argv[3]));
    }

    return 0;
}
