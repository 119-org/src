#ifndef _VIDEO_DEVICE_AGENT_H_
#define _VIDEO_DEVICE_AGENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <event2/event.h>

#include "device.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct video_device_agent {
    struct device_ctx *dev;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    int pipe_rfd;
    int pipe_wfd;
    int width;
    int height;
    struct buffer_ctx *buf_src;
    struct buffer_ctx *buf_snk;

} video_device_agent_t;


struct video_device_agent *video_device_agent_create();
int video_device_agent_dispatch(struct video_device_agent *p);
void video_device_agent_destroy(struct video_device_agent *p);

#ifdef __cplusplus
}
#endif
#endif
