#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <event2/event.h>

struct pipe_t {
    int rfd;
    int wfd;
    timer_t timerid;
    struct event_base *evbase;
    struct event *evread;
};
int skt_set_noblk(int fd, int enable)
{
    int flag;
    flag = fcntl(fd, F_GETFL);
    if (flag == -1) {
        return -1;
    }
    if (enable) {
        flag |= O_NONBLOCK;
    } else {
        flag &= ~O_NONBLOCK;
    }
    if (-1 == fcntl(fd, F_SETFL, flag)) {
        return -1;
    }
    return 0;
}
typedef void (*timer_handle_t)(union sigval sv);
timer_t timer_handle_create(timer_handle_t cb, void *arg, struct timeval *tv)
{
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    memset(&sev, 0, sizeof(struct sigevent));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = cb;
    sev.sigev_value.sival_ptr = arg;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        printf("timer_create");
        return (timer_t)(-1);
    }

    its.it_value.tv_sec = tv->tv_sec;
    its.it_value.tv_nsec = tv->tv_usec * 1000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        printf("timer_settime");
        return (timer_t)(-1);
    }
    return timerid;
}

void timer_handle_destroy(timer_t tid)
{
    timer_delete(tid);
}

static void on_pipe_read(int fd, short what, void *arg)
{
    struct pipe_t *p = (struct pipe_t *)arg;
    char buf[64] = {0};
    read(p->rfd, buf, sizeof(buf));
    printf("%s:%d buf = %s\n", __func__, __LINE__, buf);
//    sleep(10);
}

static void on_pipe_write(union sigval sv)
{
    struct pipe_t *p = (struct pipe_t *)sv.sival_ptr;
    char *buf = "pipe write";
    write(p->wfd, buf, strlen(buf));
    printf("%s:%d buf = %s\n", __func__, __LINE__, buf);
}

static void *test_pipe_loop(void *arg)
{
    struct pipe_t *p = (struct pipe_t *)arg;
    event_base_loop(p->evbase, 0);
    return NULL;
}

int test_pipe()
{
    int fds[2];
    pthread_t tid;
    struct timeval tv = {1, 0};
    struct pipe_t *p = (struct pipe_t *)calloc(1, sizeof(struct pipe_t));
    pipe(fds);
    p->rfd = fds[0];
    p->wfd = fds[1];
    skt_set_noblk(p->rfd, 1);
    skt_set_noblk(p->wfd, 1);
    p->timerid = timer_handle_create(on_pipe_write, p, &tv);
    p->evbase = event_base_new();
    p->evread = event_new(p->evbase, p->rfd, EV_READ|EV_PERSIST, on_pipe_read, p);
    event_add(p->evread, NULL);


    pthread_create(&tid, NULL, test_pipe_loop, p);
    return 0;
}

int test_event_base()
{
    struct event_base *evbase = event_base_new();
    event_base_free(evbase);
    return 0;
}

static void on_write(int fd, short what, void *arg)
{
    int rfd = *(int *)arg;
    char ch;
    read(rfd, &ch, 1);
    printf("%s:%d fd = %d, rfd = %d, ch = %c\n", __func__, __LINE__, fd, rfd, ch);
}

static void on_read(int fd, short what, void *arg)
{
    int rfd = *(int *)arg;
    char ch;
    read(rfd, &ch, 1);
    printf("%s:%d fd = %d, rfd = %d, ch = %c\n", __func__, __LINE__, fd, rfd, ch);
}


static void *thread_event_write(void *arg)
{
    int *fd = (int *)arg;
    int rfd = fd[0];
    int wfd = fd[1];
    char ch = 'a';
    while (1) {
        write(wfd, &ch, 1);
        sleep(1);
        read(rfd, &ch, 1);
        sleep(1);
    }
    return NULL;
}
int test_event_write()
{
    pthread_t tid;
    int fds[2];
    pipe(fds);
    int wfd = fds[1];
    int rfd = fds[0];
    printf("wfd = %d\n", wfd);
    struct event_base *evbase = event_base_new();
    struct event *evread = event_new(evbase, rfd, EV_WRITE|EV_PERSIST, on_read, &wfd);
    event_add(evread, NULL);
    struct event *evwrite = event_new(evbase, wfd, EV_WRITE|EV_PERSIST, on_write, &rfd);
    event_add(evwrite, NULL);
    pthread_create(&tid, NULL, thread_event_write, fds);
    event_base_loop(evbase, 0);
    return 0;
}

int main(int argc, char **argv)
{
//    test_pipe();
//    test_event_base();
    test_event_write();
    while (1) {
        sleep(10);
    }
    return 0;
}
