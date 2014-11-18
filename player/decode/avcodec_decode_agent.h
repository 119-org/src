#ifndef _AVCODEC_DECODE_AGENT_H_
#define _AVCODEC_DECODE_AGENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct avcodec_decode_agent {
    struct codec_ctx *pc;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int pipe_rfd;
    int pipe_wfd;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;
} avcodec_decode_agent_t;

struct avcodec_decode_agent *avcodec_decode_agent_create(struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk);

int avcodec_decode_agent_dispatch(struct avcodec_decode_agent *avda);
void avcodec_decode_agent_destroy(struct avcodec_decode_agent *na);


#ifdef __cplusplus
}
#endif
#endif
