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
    int pipe_rfd;
    int pipe_wfd;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;
} decoder_agent_t;

struct decoder_agent *decoder_agent_create(struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk);
void decoder_agent_destroy(struct decoder_agent *na);




#ifdef __cplusplus
}
#endif
#endif
