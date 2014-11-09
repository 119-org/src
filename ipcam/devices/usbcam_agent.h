#ifndef _USBCAM_AGENT_H_
#define _USBCAM_AGENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <event2/event.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usbcam_agent {
    struct device_ctx *dc;
    struct event_base *ev_base;
    struct event *ev_read;
    struct event *ev_close;
    timer_t timerid;
    int on_read_fd;
    int on_write_fd;
    struct queue_ctx *qout;

} usbcam_agent_t;


struct usbcam_agent *usbcam_agent_create();
int usbcam_agent_dispatch(struct usbcam_agent *ua);
void usbcam_agent_destroy(struct usbcam_agent *ua);

#ifdef __cplusplus
}
#endif
#endif
