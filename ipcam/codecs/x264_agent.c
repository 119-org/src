#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <event2/event.h>

#include "common.h"
#include "codec.h"
#include "queue.h"
#include "x264_agent.h"



static void on_break_event(int fd, short what, void *arg)
{

}

static void on_x264_read(int fd, short what, void *arg)
{
    int len;
    struct queue_item *in_item, *out_item;
    int flen = 0x10000;//bigger than one x264 packet buffer
    void *enc_buf = calloc(1, flen);
    x264_agent_t *xa = (x264_agent_t *)arg;
    struct codec_ctx *encoder = xa->encoder;
    struct queue_ctx *qin = xa->qin;
    struct queue_ctx *qout = xa->qout;

    while (NULL != (in_item = queue_pop(qin))) {
        len = codec_encode(encoder, in_item->data, enc_buf);
        if (len == -1) {
            printf("encode failed!\n");
            free(enc_buf);
            queue_item_free(in_item);
            return;
        }
        assert(len <= flen);
        out_item = queue_item_new(enc_buf, len);
        queue_push(qout, out_item);
        queue_item_free(in_item);
    }
}

static void *x264_agent_loop(void *arg)
{
    struct x264_agent *xa = (struct x264_agent *)arg;
    if (!xa)
        return NULL;
    event_base_loop(xa->ev_base, 0);
    return NULL;
}

static void on_x264_write(union sigval sv)
{
    struct queue_item *in_item, *out_item;
    int len;
    struct x264_agent *xa = (struct x264_agent *)sv.sival_ptr;
    struct queue_ctx *qout = xa->qout;

    int flen = 0x10000;//bigger than one x264 packet buffer
    void *enc_buf = calloc(1, flen);
    out_item = queue_item_new(enc_buf, len);
    queue_push(qout, out_item);
    return;
}


struct x264_agent *x264_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    x264_agent_t *xa = NULL;
    struct timeval tv = {1, 0};
    pthread_t tid;
    int fds[2];

    xa = (x264_agent_t *)calloc(1, sizeof(x264_agent_t));
    if (!xa)
        return NULL;

    xa->encoder = codec_new("x264");
    if (!xa->encoder)
        return NULL;
    if (-1 == codec_open(xa->encoder, 640, 480)) {
        printf("source_open failed!\n");
        return -1;
    }
    xa->qin = qin;
    xa->qout = qout;

    xa->ev_base = event_base_new();
    if (!xa->ev_base)
        return NULL;
#if 0
    xa->timerid = timer_handle_create(on_x264_write, xa, &tv);
    if ((timer_t)(-1) == xa->timerid) {
        return NULL;
    }
#endif


    printf("%s:%d fd = %d\n", __func__, __LINE__, qin->on_read_fd);
    xa->ev_read = event_new(xa->ev_base, qin->on_read_fd, EV_READ|EV_PERSIST, on_x264_read, xa);
    if (!xa->ev_read) {
        return NULL;
    }
    if (-1 == event_add(xa->ev_read, NULL)) {
        return NULL;
    }

    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    xa->on_read_fd = fds[0];
    xa->on_write_fd = fds[1];

    xa->ev_close = event_new(xa->ev_base, xa->on_read_fd, EV_READ|EV_PERSIST, on_break_event, xa->encoder);
    if (!xa->ev_close) {
        return NULL;
    }
    if (-1 == event_add(xa->ev_close, NULL)) {
        return NULL;
    }

    if (0 != pthread_create(&tid, NULL, x264_agent_loop, xa)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(xa);
        return NULL;
    }

    return xa;
}

static void notify_to_break_event_loop(struct x264_agent *xa)
{
    char notify = '1';
    if (!xa)
        return;
    if (write(xa->on_write_fd, &notify, 1) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}


void x264_agent_destroy(struct x264_agent *xa)
{
    if (!xa)
        return;

    if (0 != event_base_loopbreak(xa->ev_base)) {
        notify_to_break_event_loop(xa);
    }

    event_del(xa->ev_read);
    event_del(xa->ev_close);
    event_base_free(xa->ev_base);
    codec_close(xa->encoder);
    codec_free(xa->encoder);
    queue_free(xa->qin);
    queue_free(xa->qout);
    free(xa);
}
