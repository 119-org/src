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
#include "decoder_agent.h"

static void on_break_event(int fd, short what, void *arg)
{

}


static void on_buffer_read(int fd, short what, void *arg)
{
    int len;
    struct queue_item *in_item, *out_item;
    decoder_agent_t *na = (decoder_agent_t *)arg;
    struct queue_ctx *qin = na->qin;
    struct queue_ctx *qout = na->qout;
    int flen = 0x100000;//bigger than one x264 packet buffer
    void *dec_buf = calloc(1, flen);

    while (NULL != (in_item = queue_pop(qin))) {
        len = codec_decode(na->pc, in_item->data, in_item->len, &dec_buf);
        if (len == -1) {
            printf("codec_write failed!\n");
        }
        assert(len <= flen);
        out_item = queue_item_new(dec_buf, in_item->len);
        queue_push(qout, out_item);
//        queue_item_free(in_item);
//    printf("%s:%d codec_write len=%d\n", __func__, __LINE__, len);
    }
}
static void *decoder_agent_loop(void *arg)
{
    struct decoder_agent *na = (struct decoder_agent *)arg;
    if (!na)
        return NULL;
    event_base_loop(na->ev_base, 0);
    return NULL;
}


struct decoder_agent *decoder_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    pthread_t tid;
    decoder_agent_t *na = NULL;
    int fds[2];

    na = (decoder_agent_t *)calloc(1, sizeof(decoder_agent_t));
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

    na->qin = qin;
    na->qout = qout;
    na->ev_read = event_new(na->ev_base, qin->on_read_fd, EV_READ|EV_PERSIST, on_buffer_read, na);
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
    na->on_read_fd = fds[0];
    na->on_write_fd = fds[1];

    na->ev_close = event_new(na->ev_base, na->on_read_fd, EV_READ|EV_PERSIST, on_break_event, na->pc);
    if (!na->ev_close) {
        return NULL;
    }
    if (-1 == event_add(na->ev_close, NULL)) {
        return NULL;
    }

    if (0 != pthread_create(&tid, NULL, decoder_agent_loop, na)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(na);
        return NULL;
    }

    return na;
}
