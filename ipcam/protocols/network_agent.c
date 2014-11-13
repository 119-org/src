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
    int len;
    struct queue_item *item;
    network_agent_t *na = (network_agent_t *)arg;
    struct queue_ctx *qin = na->qin;

//    while (NULL != (in_item = queue_pop(qin))) {
    item = queue_pop(qin);
    if (!item) {
//        printf("%s:%d queue_pop null\n", __func__, __LINE__);
        return;
    }
    printf("%s:%d queue_pop item=%x\n", __func__, __LINE__, item->data);
    usleep(200 *1000);
    protocol_write(na->pc, item->data, item->len);
//    }
}

static void *network_agent_loop(void *arg)
{
    struct network_agent *na = (struct network_agent *)arg;
    if (!na)
        return NULL;
    event_base_loop(na->ev_base, 0);
    return NULL;
}


struct network_agent *network_agent_create(struct queue_ctx *qin, struct queue_ctx *qout)
{
    pthread_t tid;
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

    na->qin = qin;
    na->qout = qout;
    na->ev_read = event_new(na->ev_base, qin->on_read_fd, EV_READ|EV_PERSIST, on_buffer_read, na);
    if (!na->ev_read) {
        return NULL;
    }
    if (-1 == event_add(na->ev_read, NULL)) {
        return NULL;
    }

    if (0 != pthread_create(&tid, NULL, network_agent_loop, na)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(na);
        return NULL;
    }

    return na;
}




void network_agent_destroy(struct network_agent *na)
{

}
