#ifndef _X264_AGENT_H_
#define _X264_AGENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <event2/event.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct x264_agent {
    struct codec_ctx *encoder;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int pipe_rfd;
    int pipe_wfd;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;

} x264_agent_t;


struct x264_agent *x264_agent_create(struct video_device_agent *vda, struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk);
int x264_agent_dispatch(struct x264_agent *ua);
void x264_agent_destroy(struct x264_agent *ua);


#ifdef __cplusplus
}
#endif
#endif
