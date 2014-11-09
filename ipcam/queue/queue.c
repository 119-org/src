#include <stdio.h>
#include <stdlib.h>
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

struct queue_ctx *queue_new()
{
    int fds[2];
    struct queue_ctx *qc = (struct queue_ctx *)calloc(1, sizeof(struct queue_ctx));
    if (!qc) {
        return NULL;
    }
    INIT_LIST_HEAD(&qc->head);
    pthread_mutex_init(&qc->lock, NULL);
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    qc->on_read_fd = fd[0];
    qc->on_write_fd = fd[1];


    return qc;
}

int queue_add(struct queue_ctx *q, struct queue_item *item)
{
    char notify = '1';
    if (!q || !item) {
        return -1;
    }
    pthread_mutex_lock(&q->lock);
    list_add_tail(&item->entry, &q->head);
    if (write(q->on_write_fd, &notify, sizeof(notify)) != 1) {
        printf("write pipe failed: %s\n", strerror(errno));
    }
    pthread_mutex_unlock(&q->lock);
    return 0;
}

int queue_del(struct queue_ctx *q, struct queue_item *item)
{
    if (!q || !item) {
        return -1;
    }
    pthread_mutex_lock(&q->lock);
    list_del(&item->entry);
    pthread_mutex_unlock(&q->lock);
    return 0;
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
