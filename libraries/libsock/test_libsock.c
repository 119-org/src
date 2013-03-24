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

    sfd = sock_udp_server("127.0.0.1", 1234);
    if (sfd < 0) {
        printf("udp server error!\n");
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
/*设这文件句柄sfd为非阻塞
*
*/
static int make_socket_non_blocking(int sfd)
{
    int flags, s;
    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

/*创建epoll管理最大文件句柄的个数
*parameter
* @maxfds：最大的句柄个数。也就是网站数目。端口的数目
*/
int epoll_init(int maxfds)
{
    return epoll_create(maxfds);
}

/*设这epoll管理每个文件句柄的参数和方法
*parameter
* @fd：要管理socket文件句柄
* @maxfds：管理socket文件句柄的个数
*/
static struct epoll_event *epoll_event_init(int *fd, int maxfds)
{
    struct epoll_event *events;
    int i = 0;
    if (0 >= maxfds || NULL == fd)
        return NULL;
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * maxfds);
    for (; i < maxfds; i++) {
        events[i].data.fd = fd[i];
        events[i].events = EPOLLIN | EPOLLET;
    }
    return events;
}

int epoll_handler(int epfd, int *fd, int maxfds)
{
    struct epoll_event *events = epoll_event_init(fd, maxfds);
    struct epoll_event *ev = events;
    int i = 0;
    for (; i < maxfds; i++) {
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd[i], ev);
        ev++;
    }

}

/*  
@description:开始服务端监听
@parameter
ip:web服务器的地址
port：web服务器的端口
@result：成功返回创建socket套接字标识，错误返回-1
 
*/
int socket_listen(char *ip, unsigned short int port)
{
    int res_socket;             //返回值
    int res, on;
    struct sockaddr_in address;
    struct in_addr in_ip;
    res = res_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(res_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);    //inet_addr("127.0.0.1");
    res = bind(res_socket, (struct sockaddr *)&address, sizeof(address));
    if (res) {
        printf("port is used , not to repeat bind\n");
        exit(101);
    };
    res = listen(res_socket, 5);
    if (res) {
        printf("listen port is error ;\n");
        exit(102);
    };
    return res_socket;
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

#if 1
int main(int argc, char **argv)
{
//    tcp_client_run();
//    tcp_server_run();
    udp_server_run();
    return 0;
}
#endif

#if 0
int main(int argc, char *argv[])
{
    int res_socket[2];
    struct epoll_event event[100];
    struct sockaddr_in client_addr;
    int len;
    res_socket[0] = sock_tcp_server("127.0.0.1", 1234);
    res_socket[1] = sock_tcp_server("127.0.0.1", 4321);
    int epfd = epoll_create(2);
    epoll_handler(epfd, res_socket, 2);
    while (1) {
        int count = epoll_wait(epfd, event, 2, -1);
        printf("count = %d\n", count);
        while (count--) {
            sleep(1);
            int connfd = sock_tcp_accept(event[count].data.fd, 0, 0, 0);
            //针对不同的端口，即是不同的网站进行不同的处理
            if (event[count].data.fd == res_socket[0])
                send_http1(connfd);
            else
                send_http2(connfd);
            close(connfd);
        }
    }
}
#endif
