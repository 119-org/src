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
#include "event.h"

int tcp_client(const char *host, uint16_t port)
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

void handle(int fd, short flags, void *arg)
{
    printf("xxxx\n");

}

int tcp_server(uint16_t port)
{
    int fd;
    struct event_base *eb;

    fd = skt_tcp_bind_listen(NULL, port);
    if (fd == -1) {
        return -1;
    }
    skt_set_noblk(fd, 1);
    eb = event_init();
    fprintf(stderr, "%s:%d fd = %d , %p\n", __func__, __LINE__, fd, handle);
    event_add(eb, NULL, fd, 0, handle, NULL);
    event_dispatch(eb, 0);

    return 0;
}

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
    uint16_t port;
    char *ip;
    if (argc < 2) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s")) {
        if (argc == 3)
            port = atoi(argv[2]);
        else
            port = 0;
        tcp_server(port);
    } else if (!strcmp(argv[1], "-c")) {
        if (argc == 3) {
            ip = "127.0.0.1";
            port = atoi(argv[2]);
        } else if (argc == 4) {
            ip = argv[2];
            port = atoi(argv[3]);
        }
        tcp_client(ip, port);
    }
    if (!strcmp(argv[1], "-t")) {
        addr_test();
    }
    if (!strcmp(argv[1], "-d")) {
        domain_test();
    }
    return 0;
}
