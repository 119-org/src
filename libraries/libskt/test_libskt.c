#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "libskt.h"
#include "libskt_event.h"

struct conn {
    int fd;

};

int tcp_client(const char *host, uint16_t port)
{
    int n;
    char buf[] = { "hello world" };
    struct skt_connection *sc;

    sc = skt_tcp_connect(host, port);
    if (sc == NULL) {
        printf("connect failed!\n");
        return -1;
    }
    while (1) {
        n = skt_send(sc->fd, buf, strlen(buf));
        if (n == -1) {
            printf("skt_send failed!\n");
            return -1;
        }
//        sleep(1);
    }
}

void recv_msg(void *arg)
{
    int fd = *(int *)arg;
    char buf[100];
    skt_recv(fd, buf, 100);
    printf("recv buf = %s\n", buf);

}

void handle_new_conn(void *arg)
{
    struct conn *c = (struct conn *)arg;
    int fd;
    uint32_t ip;
    uint16_t port;
    fd = skt_accept(c->fd, &ip, &port);
    if (fd == -1) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return;
    };
    struct skt_ev_cbs *evcb = (struct skt_ev_cbs *)calloc(1, sizeof(struct skt_ev_cbs));
    evcb->ev_in = recv_msg;
    evcb->ev_out = NULL;
    evcb->ev_err = NULL;
    struct skt_ev *e = skt_ev_create(fd, EVENT_READ, evcb, (void *)&fd);
    if (-1 == skt_ev_add(e)) {
        printf("event_add failed!\n");
    }
}

int tcp_server(uint16_t port)
{
    int fd;
    int ret;
    struct conn *c = (struct conn *)calloc(1, sizeof(struct conn));

    fd = skt_tcp_bind_listen(NULL, port, 1);
    if (fd == -1) {
        return -1;
    }
    c->fd = fd;
    ret = skt_ev_init();
    if (ret == -1) {
        return -1;
    }

    struct skt_ev_cbs *evcb = (struct skt_ev_cbs *)calloc(1, sizeof(struct skt_ev_cbs));
    evcb->ev_in = handle_new_conn;
    evcb->ev_out = NULL;
    evcb->ev_err = NULL;
    struct skt_ev *e = skt_ev_create(fd, EVENT_READ, evcb, (void *)c);
    if (-1 == skt_ev_add(e)) {
        printf("event_add failed!\n");
    }
    skt_ev_dispatch();

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
