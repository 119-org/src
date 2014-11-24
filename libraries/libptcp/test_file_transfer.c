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

static FILE *g_rfp, *g_sfp;

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
        file_send(argv[2], udp_client_init, udp_client_send, udp_close);
    }
    if (!strcmp(argv[1], "-r")) {
//        file_recv(argv[2], tcp_server_init, tcp_server_recv, tcp_close);
        file_recv(argv[2], udp_server_init, udp_server_recv, udp_close);
    }
    return 0;
}
