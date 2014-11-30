#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>


#include <assert.h>
#include "libptcp.h"

static FILE *g_rfp, *g_sfp;
static long g_flen;

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


long filesize(FILE *fd)
{
    long curpos, length;
    curpos = ftell(fd);
    fseek(fd, 0L, SEEK_END);
    length = ftell(fd);
    fseek(fd, curpos, SEEK_SET);
    return length;
}

int tcp_server_init(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    socklen_t len = sizeof(si);

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("bind: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        printf("listen: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    int afd = accept(fd, (struct sockaddr *)&si, &len);
    if (afd == -1) {
        printf("accept: %s\n", strerror(errno));
        return -1;
    }
    return afd;
}

int tcp_client_init(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return -1;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

int tcp_client_send(int fd, void *buf, size_t len)
{
    return send(fd, buf, len, 0);
}

int tcp_server_recv(int fd, void *buf, size_t len)
{
    return recv(fd, buf, len, 0);
}

int tcp_close(int fd)
{
    return close(fd);
}

int udp_server_init(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return -1;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

int udp_client_init(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return -1;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

int udp_client_send(int fd, void *buf, size_t len)
{
    return send(fd, buf, len, 0);
}

int udp_server_recv(int fd, void *buf, size_t len)
{
    return recv(fd, buf, len, 0);
}

int udp_close(int fd)
{
    return close(fd);
}

static void recv_msg(struct my_struct *my)
{
    int len, wlen;
    char buf[1024] = {0};

    ptcp_read(my->ps, buf, 0, NULL);
    len = ptcp_recv(my->ps, buf, sizeof(buf));
    if (len <= 0) {
        printf("ptcp_recv failed\n");
    } else {
        printf("ptcp_recv len=%d\n", len);
        wlen = fwrite(buf, 1, len, g_rfp);
        assert(len==wlen);
    }
}

ptcp_socket_t *ptcp_server_init(const char *host, uint16_t port)
{
    int i, ret, epfd;
    struct epoll_event event;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    struct sockaddr_in si;
    pthread_t tid;

    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));

    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return NULL;
    }
    struct my_struct *my = (struct my_struct *)calloc(1, sizeof(*my));
    my->ps = ps;
    my->fd = ptcp_get_fd(ps);
    my->epfd = epfd;
    memset(&event, 0, sizeof(event));
    event.data.ptr = my;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, my->fd, &event)) {
        perror("epoll_ctl");
        close(epfd);
        return NULL;
    }
//    pthread_create(&tid, NULL, ptcp_server_loop, my);
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
    return ps;
}

void cli_recv_msg(struct my_struct *my)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    char buf[1024] = {0};
    ptcp_read(my->ps, buf, 0, NULL);
}

static void *cli_send_thread(void *arg)
{
    struct ptcp_socket_t *ps = (struct ptcp_socket_t *)arg;
    char buf[1024] = {0};
    int i, len, slen;
    int flen = g_flen;
    sleep(1);
    while (flen > 0) {
        usleep(100 * 1000);
        len = fread(buf, 1, sizeof(buf), g_sfp);
        if (len == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            ptcp_close(ps);
            fclose(g_sfp);
            return -1;
        }
        slen = ptcp_send(ps, buf, len);
        printf("%s:%d slen=%d, len=%d\n", __func__, __LINE__, slen, len);
        flen -= len;
    }

    ptcp_close(ps);
}



ptcp_socket_t *ptcp_client_init(const char *host, uint16_t port)
{
    pthread_t tid;
    int ret;
    int epfd;
    struct epoll_event event;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    int i;
    struct sockaddr_in si;
    
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    if (0 != ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }

    struct my_struct *my = (struct my_struct *)calloc(1, sizeof(*my));
    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return NULL;
    }
    
    my->ps = ps;
    my->fd = ptcp_get_fd(ps);
    memset(&event, 0, sizeof(event));
    event.data.ptr = my;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, my->fd, &event)) {
        perror("epoll_ctl");
        close(epfd);
        return NULL;
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
    return ps;

}

int ptcp_file_send(char *name)
{
    int i = 0;
    int len, flen, slen, total;
    char buf[1024] = {0};
    FILE *fp = fopen(name, "r");
    assert(fp);
    g_sfp = fp;
    g_flen = total = flen = filesize(fp);
    ptcp_socket_t *ps = ptcp_client_init("127.0.0.1", 5555);

    sleep(1);
    while (flen > 0) {
        usleep(100 * 1000);
        len = fread(buf, 1, sizeof(buf), fp);
        if (len == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            ptcp_close(ps);
            fclose(fp);
            return -1;
        }
        slen = ptcp_send(ps, buf, len);
        printf("%s:%d slen=%d, len=%d\n", __func__, __LINE__, slen, len);
        flen -= len;
    }
    sleep(1);
    ptcp_close(ps);
    fclose(fp);
    printf("file %s length is %u\n", name, flen);
    return total;
}

int file_send(char *name,
              int (xfer_init)(const char *host, uint16_t port),
              int (xfer_send)(int fd, void *buf, size_t len),
              int (xfer_close(int fd)))
{
    int i = 0;
    int len, flen, slen, total;
    char buf[128] = {0};
    FILE *fp = fopen(name, "r");
    assert(fp);
    g_sfp = fp;
    total = flen = filesize(fp);
    int xfd = xfer_init("127.0.0.1", 5555);

    while (flen > 0) {
        len = fread(buf, 1, sizeof(buf), fp);
        if (len == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            xfer_close(xfd);
            fclose(fp);
            return -1;
        }
        slen = xfer_send(xfd, buf, len);
        assert(slen==len);
        flen -= len;
    }
    sleep(1);
    xfer_close(xfd);
    fclose(fp);
    printf("file %s length is %u\n", name, flen);
    return total;
}
int ptcp_file_recv(char *name)
{
    int i = 0;
    int len, flen, rlen;
    char buf[128] = {0};
    FILE *fp = fopen(name, "w");
    assert(fp);
    g_rfp = fp;
    ptcp_socket_t *ps = ptcp_server_init("127.0.0.1", 5555);
    assert(ps!=NULL);

    flen = 0;
    while (1) {
        sleep(1);
#if 0
        memset(buf, 0, sizeof(buf));
        rlen = xfer_recv(xfd, buf, sizeof(buf));
        if (rlen == 0) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            xfer_close(xfd);
            fclose(fp);
            break;
        } else if (rlen == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            xfer_close(xfd);
            fclose(fp);
            return -1;
        }
        len = fwrite(buf, 1, rlen, fp);
        assert(len==rlen);
        flen += len;
#endif
    }
    return flen;
}

int file_recv(char *name,
              int (xfer_init)(const char *host, uint16_t port),
              int (xfer_recv)(int fd, void *buf, size_t len),
              int (xfer_close(int fd)))
{
    int i = 0;
    int len, flen, rlen;
    char buf[128] = {0};
    FILE *fp = fopen(name, "w");
    assert(fp);
    g_rfp = fp;
    int xfd = xfer_init("127.0.0.1", 5555);
    assert(xfd!=-1);

    flen = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        rlen = xfer_recv(xfd, buf, sizeof(buf));
        if (rlen == 0) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            xfer_close(xfd);
            fclose(fp);
            break;
        } else if (rlen == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            xfer_close(xfd);
            fclose(fp);
            return -1;
        }
        len = fwrite(buf, 1, rlen, fp);
        assert(len==rlen);
        flen += len;
    }
    return flen;
}

static void sigterm_handler(int sig)
{
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("./test -s / -r filename\n");
        return 0;
    }
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, sigterm_handler);


    if (!strcmp(argv[1], "-s")) {
//        file_send(argv[2], tcp_client_init, tcp_client_send, tcp_close);
//        file_send(argv[2], udp_client_init, udp_client_send, udp_close);
        ptcp_file_send(argv[2]);
    }
    if (!strcmp(argv[1], "-r")) {
//        file_recv(argv[2], tcp_server_init, tcp_server_recv, tcp_close);
//        file_recv(argv[2], udp_server_init, udp_server_recv, udp_close);
        ptcp_file_recv(argv[2]);
    }
    return 0;
}
