#ifndef _YQUEUE_H_
#define _YQUEUE_H_

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "kernel_list.h"
#include "atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * YQUEUE is a multi reader single writer queue, like "Y"
 * reader number is fixed
 */

struct yqueue_item {
    struct list_head entry;
    void *data;
    int len;
    struct atomic ref;
    int last_ref;
};

struct pipe_fds {
    int rfd; //pipe fds[0]
    int wfd; //pipe fds[1]
    int used;
    struct list_head entry;
};

struct priv_params {
    struct {
        int width;
        int height;
    } video;
    struct {
    } audio;
};


struct yqueue {
    struct list_head head;
    pthread_mutex_t lock;
    int ref_cnt;//every item ref_cnt is fixed
    int depth;//current depth
    int max_depth;
    struct list_head pipe_list;
    struct priv_params *media_info;
};

struct yqueue_item *yqueue_item_new(void *data, int len);
void yqueue_item_free(struct yqueue_item *item);

struct yqueue *yqueue_create();
int yqueue_add_ref(struct yqueue *q);
int yqueue_get_available_fd(struct yqueue *q);
int yqueue_push(struct yqueue *q, struct yqueue_item *item);
struct yqueue_item *yqueue_pop(struct yqueue *q, int fd, int *last);
int yqueue_depth(struct yqueue *q);
void yqueue_destroy(struct yqueue *q);


#ifdef __cplusplus
}
#endif
#endif
