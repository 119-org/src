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
    int on_read_fd;
    int on_write_fd;
    struct queue_ctx *qin;
    struct queue_ctx *qout;
} display_agent_t;


struct display_agent *display_agent_create(struct queue_ctx *qin, struct queue_ctx *qout);
void display_agent_destroy(struct display_agent *na);



#ifdef __cplusplus
}
#endif
#endif
