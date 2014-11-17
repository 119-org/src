#ifndef _DISPLAY_AGENT_H_
#define _DISPLAY_AGENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct display_agent {
    struct protocol_ctx *pc;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int pipe_rfd;
    int pipe_wfd;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;
} display_agent_t;


struct display_agent *display_agent_create(struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk);
void display_agent_destroy(struct display_agent *na);



#ifdef __cplusplus
}
#endif
#endif
