#ifndef _DECODER_AGENT_H_
#define _DECODER_AGENT_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef struct decoder_agent {
    struct codec_ctx *pc;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int on_read_fd;
    int on_write_fd;
    struct queue_ctx *qin;
    struct queue_ctx *qout;
} decoder_agent_t;

struct decoder_agent *decoder_agent_create(struct queue_ctx *qin, struct queue_ctx *qout);
void decoder_agent_destroy(struct decoder_agent *na);




#ifdef __cplusplus
}
#endif
#endif
