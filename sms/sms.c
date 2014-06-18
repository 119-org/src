#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>


#include "libskt/libskt.h"
#include "libmempool/libmempool.h"
#include "sms.h"

#define NGX_CONF_UNSET_SIZE  (size_t) -1
#define EPOLL_MAX_NEVENT	4096
#define RECV_BUFLEN		1024
#define LISTEN_MAX_BACKLOG	128


struct sms		g_sms;
struct skt_conn		g_conn;
struct mempool		*g_mp;
struct threadpool	*g_tp;

static int on_send_msg(int fd)
{
    int n;
    char buf[20] = {0};

    sprintf(buf, "%d:%s", fd, "haha");
    n = skt_send(fd, buf, sizeof(buf));
    if (n == -1) {
        fprintf(stderr, "send failed!\n");
        return -1;
    }
    return 0;
}

static void handle_request(void *args)
{
    struct buffer *req_buf = (struct buffer *)args;

    return NULL;
}

static int on_recv(int fd)
{
    int n;
    struct buffer *recvbuf;
    void *p;
    p = (void *)mp_alloc(g_mp, RECV_BUFLEN);
    recvbuf = (struct buffer *)mp_alloc(g_mp, sizeof(struct buffer));
    recvbuf->addr = p;
    recvbuf->len = RECV_BUFLEN;
    recvbuf->ref_cnt = 0;

    n = skt_recv(fd, p, RECV_BUFLEN);
    if (n == -1) {
        fprintf(stderr, "recv failed!\n");
        return -1;
    }
    tp_add(g_tp, handle_request, recvbuf); 
    fprintf(stderr, "fd = %d, recv = %s\n", fd, p);
    sleep(1);
    n = on_send_msg(fd);
    if (n == -1) {
        fprintf(stderr, "send failed!\n");
        return -1;
    }
    mp_free(g_mp, p);
    return 0;
}


static epop_t *epoll_init(skt_conn_t *c)
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

    epop->on_recv = on_recv;
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
                    fprintf(stderr, "on_recv failed!\n");
                }
            } else if (events[i].events & EPOLLOUT) {
//FIXME
//doesn't trigger because EPOLLOUT isn't registered
                fprintf(stderr, "%s:%d\n", __func__, __LINE__);
                if (-1 == epop->on_send(events[i].data.fd)) {
                    fprintf(stderr, "on_send failed!\n");
                }

            } else {
                printf("%s:%d\n", __func__, __LINE__);
                close(events[i].data.fd);
            }
        }
    }
    return NULL;
}

int sms_init()
{
    int fd;
    uint16_t port = RTSP_PORT;
    pthread_t tid;
    epop_t *epop;

    skt_init();
    fd = skt_tcp_binden(&g_conn, port);
    if (fd == -1) {
        fprintf(stderr, "create listen socket failed!\n");
        return -1;
    }
    g_sms.socket = fd;
    if (NULL == (epop = epoll_init(&g_conn))) {
        fprintf(stderr, "epoll_init failed!\n");
        return -1;
    }

    if (0 != pthread_create(&tid, NULL, epoll_dispatch, (void *)epop)) {
        fprintf(stderr, "pthread_create failed!\n");
        return -1;
    }

//memory pool for on_recv message buffer
    g_mp = mp_init();
    if (g_mp == NULL) {
        fprintf(stderr, "create memory pool failed!\n");
        return -1;
    }

//thread pool for on_recv message parse
    g_tp = tp_init();
    if (g_tp == NULL) {
        fprintf(stderr, "create thread pool failed!\n");
        return -1;
    }

    return 0;
}

int sms_loop()
{
    while (1) {
        sleep(1);
    }

    return 0;
}

int main(int argc, char **argv)
{
    sms_init();
    sms_loop();

    return 0;
}
