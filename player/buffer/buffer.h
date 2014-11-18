#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "kernel_list.h"
#include "atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

enum media_type {
    MEDIA_TYPE_UNKNOWN = -1,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,

};

struct buf_video_props {
    int width;
    int height;
};

struct buf_audio_props {
    int sample_rate;

};

struct buffer_item {
    struct list_head entry;
    void *data;
    int len;
    struct atomic ref;
};

struct buffer_ctx {
    struct list_head head;
    pthread_mutex_t lock;
    int depth;
    int max_depth;
    int pipe_wfd;
    int pipe_rfd;
};

struct buffer_item *buffer_item_new(void *data, int len);
void buffer_item_free(struct buffer_item *item);
void buffer_item_ref_add(struct buffer_item *item);
void buffer_item_ref_sub(struct buffer_item *item);

struct buffer_ctx *buffer_create(int max);
int buffer_push(struct buffer_ctx *b, struct buffer_item *item);
struct buffer_item *buffer_pop(struct buffer_ctx *b);
int buffer_resize_max_depth(struct buffer_ctx *b, int max);
int buffer_get_depth(struct buffer_ctx *b);
int buffer_is_empty(struct buffer_ctx *b);
void buffer_destroy(struct buffer_ctx *b);


#ifdef __cplusplus
}
#endif
#endif
