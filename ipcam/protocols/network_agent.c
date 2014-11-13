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
#include "protocol.h"
#include "queue.h"
#include "network_agent.h"

static void on_buffer_read(int fd, short what, void *arg)
{

}

struct network_agent *network_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    network_agent_t *na = NULL;

    na = (network_agent_t *)calloc(1, sizeof(network_agent_t));
    if (!na) {
        return NULL;
    }
    na->pc = protocol_init("udp://127.0.0.1:2333");
    if (!na->pc) {
        return NULL;
    }
    if (-1 == protocol_open(na->pc)) {
        return NULL;
    }
    na->ev_base = event_base_new();
    if (!na->ev_base) {
        return NULL;
    }
    na->ev_read = event_new(na->ev_base, qin->on_read_fd, EV_READ|EV_PERSIST, on_network_read, na);
    if (!na->ev_read) {
        return NULL;
    }


    return na;
}




void network_agent_destroy(struct network_agent *na)
{

}
