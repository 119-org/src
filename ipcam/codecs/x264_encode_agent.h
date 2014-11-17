#ifndef _X264_ENCODE_AGENT_H_
#define _X264_ENCODE_AGENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <event2/event.h>
#include "video_capture_agent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct x264_encode_agent {
    struct codec_ctx *encoder;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int pipe_rfd;
    int pipe_wfd;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;

} x264_encode_agent_t;


struct x264_encode_agent *x264_encode_agent_create(struct video_capture_agent *vca, struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk);
int x264_encode_agent_dispatch(struct x264_encode_agent *ua);
void x264_encode_agent_destroy(struct x264_encode_agent *ua);


#ifdef __cplusplus
}
#endif
#endif
