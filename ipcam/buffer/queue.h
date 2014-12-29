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

struct queue {
    struct list_head head;
    pthread_mutex_t lock;
    int depth;
    int max_depth;
    int pipe_wfd;
    int pipe_rfd;
};


struct queue_item *queue_item_new(void *data, int len);
void queue_item_free(struct queue_item *item);

struct queue *queue_new(int max);
int queue_push(struct queue *q, struct queue_item *item);
struct queue_item *queue_pop(struct queue *q);
void queue_free(struct queue *q);
int queue_depth(struct queue *q);


#ifdef __cplusplus
}
#endif
#endif
