#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "atomic.h"
#include "buffer.h"

void buffer_item_add_ref(struct buffer_item *item)
{
    atomic_inc(&item->ref);
}

void buffer_item_sub_ref(struct buffer_item *item)
{
    if (0 < atomic_get(&item->ref)) {
        atomic_dec(&item->ref);
    }
}

int buffer_item_get_ref(struct buffer_item *item)
{
    return atomic_get(&item->ref);
}

struct buffer_item *buffer_item_new(void *data, int len)
{
    struct buffer_item *item = (struct buffer_item *)calloc(1, sizeof(struct buffer_item));
    if (!item) {
        return NULL;
    }
    item->data = data;
    item->len = len;
    atomic_set(&item->ref, 0);
    return item;
}

void buffer_item_free(struct buffer_item *item)
{
    if (!item) {
        return;
    }
    buffer_item_sub_ref(item);
    int ret = buffer_item_get_ref(item);
    if (0 != ret) {
        printf("warning: item is referenced, can't free!\n");
        return;
    }
    free(item->data);
    free(item);
    item = NULL;
}


/************************************/
struct buffer_ctx *buffer_create(int max)
{
    int fds[2];
    struct buffer_ctx *q;
    if (max < 1) {
        printf("warning: buffer max depth must > 0\n");
        max = 1;
    }
    q = (struct buffer_ctx *)calloc(1, sizeof(struct buffer_ctx));
    if (!q) {
        return NULL;
    }
    INIT_LIST_HEAD(&q->head);
    pthread_mutex_init(&q->lock, NULL);
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    q->depth = 0;
    q->max_depth = max;
    q->pipe_rfd = fds[0];
    q->pipe_wfd = fds[1];

    return q;
}

int buffer_push(struct buffer_ctx *q, struct buffer_item *item)
{
    char notify = '1';
    if (!q || !item) {
        return -1;
    }
    pthread_mutex_lock(&q->lock);
    list_add_tail(&item->entry, &q->head);
    ++(q->depth);
    if (write(q->pipe_wfd, &notify, sizeof(notify)) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
    pthread_mutex_unlock(&q->lock);
    if (q->depth > q->max_depth) {
        buffer_pop(q);
    }
    return 0;
}

struct buffer_item *buffer_pop(struct buffer_ctx *q)
{
    if (!q) {
        return NULL;
    }
    if (list_empty(&q->head)) {
        return NULL;
    }
    char notify;
    struct buffer_item *item = NULL;
    pthread_mutex_lock(&q->lock);
    item = list_first_entry_or_null(&q->head, struct buffer_item, entry);
    if (item) {
        list_del(&item->entry);
        --(q->depth);
        if (read(q->pipe_rfd, &notify, sizeof(notify)) != 1) {
            printf("read pipe failed: %s\n", strerror(errno));
        }
    }
    pthread_mutex_unlock(&q->lock);
    return item;
}

int buffer_is_empty(struct buffer_ctx *q)
{
    if (!q) {
        return 1;
    }
    return (q->depth == 0);
}

int buffer_get_depth(struct buffer_ctx *q)
{
    if (!q) {
        return 0;
    }
    return q->depth;
}

void buffer_destroy(struct buffer_ctx *q)
{
    if (!q) {
        return;
    }
    struct buffer_item *item, *next;

    list_for_each_entry_safe(item, next, &q->head, entry) {
        list_del(&item->entry);
        buffer_item_free(item);
    }
    pthread_mutex_destroy(&q->lock);
    close(q->pipe_rfd);
    close(q->pipe_wfd);
    free(q);
    q = NULL;
}
