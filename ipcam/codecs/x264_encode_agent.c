#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <x264.h>
#include <event2/event.h>

#include "common.h"
#include "codec.h"
#include "buffer.h"
#include "video_capture_agent.h"
#include "x264_encode_agent.h"

static void on_break_event(int fd, short what, void *arg)
{

}

static void on_x264_read(int fd, short what, void *arg)
{
    int len;
    struct buffer_item *in_item, *out_item;
    void *enc_buf;
    x264_encode_agent_t *xa = (x264_encode_agent_t *)arg;
    struct codec_ctx *encoder = xa->encoder;
    struct buffer_ctx *buf_src = xa->buf_src;
    struct buffer_ctx *buf_snk = xa->buf_snk;

    while (NULL != (in_item = buffer_pop(buf_src))) {
        len = codec_encode(encoder, in_item->data, &enc_buf);
        if (len == -1) {
            printf("encode failed!\n");
            buffer_item_free(in_item);
            return;
        }
        out_item = buffer_item_new(enc_buf, len);
        buffer_push(buf_snk, out_item);
        buffer_item_free(in_item);
    }
}

static void notify_to_break_event_loop(struct x264_encode_agent *xa)
{
    char notify = '1';
    if (!xa)
        return;
    if (write(xa->pipe_wfd, &notify, 1) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}


static void *x264_encode_agent_loop(void *arg)
{
    struct x264_encode_agent *xa = (struct x264_encode_agent *)arg;
    event_base_loop(xa->ev_base, 0);
    return NULL;
}

struct x264_encode_agent *x264_encode_agent_create(struct video_capture_agent *vca, struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk)
{
    x264_encode_agent_t *xa = NULL;
    int fds[2];

    xa = (x264_encode_agent_t *)calloc(1, sizeof(x264_encode_agent_t));
    if (!xa)
        return NULL;

    xa->encoder = codec_new("x264");
    if (!xa->encoder)
        return NULL;
    if (-1 == codec_open(xa->encoder, vca->dev->video.width, vca->dev->video.height)) {
        printf("source_open failed!\n");
        return NULL;
    }
    xa->buf_src = buf_src;
    xa->buf_snk = buf_snk;

    xa->ev_base = event_base_new();
    if (!xa->ev_base)
        return NULL;

    xa->ev_read = event_new(xa->ev_base, buf_src->pipe_rfd, EV_READ|EV_PERSIST, on_x264_read, xa);
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
    xa->pipe_rfd = fds[0];
    xa->pipe_wfd = fds[1];

    xa->ev_close = event_new(xa->ev_base, xa->pipe_rfd, EV_READ|EV_PERSIST, on_break_event, xa->encoder);
    if (!xa->ev_close) {
        return NULL;
    }
    if (-1 == event_add(xa->ev_close, NULL)) {
        return NULL;
    }
    return xa;
}

int x264_encode_agent_dispatch(struct x264_encode_agent *xea)
{
    pthread_t tid;
    if (0 != pthread_create(&tid, NULL, x264_encode_agent_loop, xea)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(xea);
        return -1;
    }
    return 0;
}

void x264_encode_agent_destroy(struct x264_encode_agent *xa)
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
    free(xa);
}
