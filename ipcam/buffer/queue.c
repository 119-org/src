#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "queue.h"

struct queue_item *queue_item_new(void *data, int len)
{
    struct queue_item *item = (struct queue_item *)calloc(1, sizeof(struct queue_item));
    if (!item) {
        return NULL;
    }
    item->data = data;
    item->len = len;
    return item;
}

void queue_item_free(struct queue_item *item)
{
    if (!item) {
        return;
    }
    free(item->data);
    free(item);
}

struct queue_ctx *queue_new(int max)
{
    int fds[2];
    struct queue_ctx *q = (struct queue_ctx *)calloc(1, sizeof(struct queue_ctx));
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
    q->on_read_fd = fds[0];
    q->on_write_fd = fds[1];

    return q;
}

int queue_push(struct queue_ctx *q, struct queue_item *item)
{
    char notify = '1';
    if (!q || !item) {
        return -1;
    }
    pthread_mutex_lock(&q->lock);
    list_add_tail(&item->entry, &q->head);
    ++(q->depth);
    if (write(q->on_write_fd, &notify, sizeof(notify)) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
    pthread_mutex_unlock(&q->lock);
    if (q->depth > q->max_depth) {
        queue_pop(q);
    }
    return 0;
}

struct queue_item *queue_pop(struct queue_ctx *q)
{
    if (!q) {
        return NULL;
    }
    if (list_empty(&q->head)) {
        return NULL;
    }
    char notify;
    struct queue_item *item = NULL;
    pthread_mutex_lock(&q->lock);
    item = list_first_entry_or_null(&q->head, struct queue_item, entry);
    if (item) {
        list_del(&item->entry);
        --(q->depth);
        if (read(q->on_read_fd, &notify, sizeof(notify)) != 1) {
            printf("read pipe failed: %s\n", strerror(errno));
        }
    }
    pthread_mutex_unlock(&q->lock);
    return item;
}

int queue_empty(struct queue_ctx *q)
{
    if (!q) {
        return 1;
    }
    return (q->depth == 0);
}

int queue_depth(struct queue_ctx *q)
{
    if (!q) {
        return 0;
    }
    return q->depth;
}

void queue_free(struct queue_ctx *q)
{
    struct queue_item *item, *next;
    if (!q) {
        return;
    }

    list_for_each_entry_safe(item, next, &q->head, entry) {
        list_del(&item->entry);
        queue_item_free(item);
    }
    pthread_mutex_destroy(&q->lock);
    close(q->on_read_fd);
    close(q->on_write_fd);
    free(q);
}
