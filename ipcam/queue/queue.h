#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "kernel_list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct queue_item {
    struct list_head entry;
    void *data;
    int len;
};

struct queue_ctx {
    struct list_head head;
    pthread_mutex_t lock;
    int on_write_fd;
    int on_read_fd;
};


struct queue_item *queue_item_new(void *data, int len);
void queue_item_free(struct queue_item *item);

struct queue_ctx *queue_new();
int queue_add(struct queue_ctx *q, struct queue_item *item);
int queue_del(struct queue_ctx *q, struct queue_item *item);
void queue_free(struct queue_ctx *q);

#ifdef __cplusplus
}
#endif
#endif
