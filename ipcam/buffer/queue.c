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

struct queue *queue_new(int max)
{
    int fds[2];
    struct queue *q;
    if (max < 1) {
        printf("warning: queue max depth must > 0\n");
        max = 1;
    }
    q = (struct queue *)calloc(1, sizeof(struct queue));
    if (!q) {
        return NULL;
    }
    INIT_LIST_HEAD(&q->head);
    pthread_mutex_init(&q->lock, NULL);
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        free(q);
        return NULL;
    }
    q->depth = 0;
    q->max_depth = max;
    q->pipe_rfd = fds[0];
    q->pipe_wfd = fds[1];

    return q;
}

int queue_push(struct queue *q, struct queue_item *item)
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
        queue_pop(q);
    }
    return 0;
}

struct queue_item *queue_pop(struct queue *q)
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
        if (read(q->pipe_rfd, &notify, sizeof(notify)) != 1) {
            printf("read pipe failed: %s\n", strerror(errno));
        }
    }
    pthread_mutex_unlock(&q->lock);
    return item;
}

int queue_empty(struct queue *q)
{
    if (!q) {
        return 1;
    }
    return (q->depth == 0);
}

int queue_depth(struct queue *q)
{
    if (!q) {
        return 0;
    }
    return q->depth;
}

void queue_free(struct queue *q)
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
    close(q->pipe_rfd);
    close(q->pipe_wfd);
    free(q);
    q = NULL;
}
