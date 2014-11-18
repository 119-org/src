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
#include "buffer.h"
#include "avcodec_decode_agent.h"

static void on_break_event(int fd, short what, void *arg)
{

}


static void on_buffer_read(int fd, short what, void *arg)
{
    int len;
    struct buffer_item *in_item, *out_item;
    avcodec_decode_agent_t *na = (avcodec_decode_agent_t *)arg;
    struct buffer_ctx *buf_src = na->buf_src;
    struct buffer_ctx *buf_snk = na->buf_snk;
    int flen = 0x100000;//bigger than one x264 packet buffer
    void *dec_buf = calloc(1, flen);

    while (NULL != (in_item = buffer_pop(buf_src))) {
        len = codec_decode(na->pc, in_item->data, in_item->len, &dec_buf);
        if (len == -1) {
            printf("codec_write failed!\n");
        }
        assert(len <= flen);
        out_item = buffer_item_new(dec_buf, in_item->len);
        buffer_push(buf_snk, out_item);
//        buffer_item_free(in_item);
//    printf("%s:%d codec_write len=%d\n", __func__, __LINE__, len);
    }
}
static void *avcodec_decode_agent_loop(void *arg)
{
    struct avcodec_decode_agent *na = (struct avcodec_decode_agent *)arg;
    if (!na)
        return NULL;
    event_base_loop(na->ev_base, 0);
    return NULL;
}
static void break_event_base_loop(struct avcodec_decode_agent *avda)
{
    char notify = '1';
    if (!avda)
        return;
    if (write(avda->pipe_wfd, &notify, sizeof(notify)) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
}

struct avcodec_decode_agent *avcodec_decode_agent_create(struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk)
{
    pthread_t tid;
    avcodec_decode_agent_t *na = NULL;
    int fds[2];

    na = (avcodec_decode_agent_t *)calloc(1, sizeof(avcodec_decode_agent_t));
    if (!na) {
        return NULL;
    }
    na->pc = codec_new("avcodec");
    if (!na->pc) {
        return NULL;
    }
    if (-1 == codec_open(na->pc, 640, 480)) {
        return NULL;
    }
    na->ev_base = event_base_new();
    if (!na->ev_base) {
        return NULL;
    }

    na->buf_src = buf_src;
    na->buf_snk = buf_snk;
    na->ev_read = event_new(na->ev_base, buf_src->pipe_rfd, EV_READ|EV_PERSIST, on_buffer_read, na);
    if (!na->ev_read) {
        return NULL;
    }
    if (-1 == event_add(na->ev_read, NULL)) {
        return NULL;
    }
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    na->pipe_rfd = fds[0];
    na->pipe_wfd = fds[1];

    na->ev_close = event_new(na->ev_base, na->pipe_rfd, EV_READ|EV_PERSIST, on_break_event, na->pc);
    if (!na->ev_close) {
        return NULL;
    }
    if (-1 == event_add(na->ev_close, NULL)) {
        return NULL;
    }

    return na;
}

int avcodec_decode_agent_dispatch(struct avcodec_decode_agent *avda)
{
    pthread_t tid;

    if (0 != pthread_create(&tid, NULL, avcodec_decode_agent_loop, avda)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(avda);
        return -1;
    }
    return 0;
}


void avcodec_decode_agent_destroy(struct avcodec_decode_agent *avda)
{
    if (!avda)
        return;

    if (0 != event_base_loopbreak(avda->ev_base)) {
        break_event_base_loop(avda);
    }

    event_del(avda->ev_read);
    event_del(avda->ev_close);
    event_base_free(avda->ev_base);
    codec_close(avda->pc);
    codec_free(avda->pc);
    free(avda);
}
