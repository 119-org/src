#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "yqueue.h"

#define YQUEUE_MAX_DEPTH 10

struct yqueue_item *yqueue_item_new(void *data, int len)
{
    struct yqueue_item *item = (struct yqueue_item *)calloc(1, sizeof(struct yqueue_item));
    if (!item) {
        return NULL;
    }
    item->data = data;
    item->len = len;
    item->last_ref = 0;
    return item;
}

void yqueue_item_free(struct yqueue_item *item)
{
    if (!item) {
        return;
    }
    if (atomic_get(&item->ref) > 0) {
        return;
    }
    if (item->data) {
        //printf("free data %p\n", item->data);
        free(item->data);
        item->data = NULL;
    }
    if (item) {
        //printf("free item %p\n", item);
        free(item);
        item = NULL;
    }
}

int yqueue_get_available_fd(struct yqueue *q)
{
    struct pipe_fds *pipe_item, *next;
    list_for_each_entry_safe(pipe_item, next, &q->pipe_list, entry) {
        if (!pipe_item->used) {
            pipe_item->used = 1;
            return pipe_item->rfd;
        }
    }
    return -1;
}

int yqueue_add_ref(struct yqueue *q)
{
    if (!q) {
        return -1;
    }
    int fds[2];
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return -1;
    }
    struct pipe_fds *pfds = (struct pipe_fds *)calloc(1, sizeof(struct pipe_fds));
    pfds->rfd = fds[0];
    pfds->wfd = fds[1];
    pfds->used = 0;
    list_add_tail(&pfds->entry, &q->pipe_list);
    q->ref_cnt++;
    return 0;
}

struct yqueue_item *yqueue_pop_absolute(struct yqueue *q)
{
    if (!q) {
        return NULL;
    }
    if (list_empty(&q->head)) {
        return NULL;
    }
    char notify;
    struct yqueue_item *item = NULL;
    struct pipe_fds *pipe_item, *next;
    pthread_mutex_lock(&q->lock);
    item = list_first_entry_or_null(&q->head, struct yqueue_item, entry);
    if (item) {
        list_del(&item->entry);
        --(q->depth);
        list_for_each_entry_safe(pipe_item, next, &q->pipe_list, entry) {
            if (read(pipe_item->rfd, &notify, sizeof(notify)) != 1) {
                printf("read pipe fd = %d failed: %s\n", pipe_item->rfd, strerror(errno));
            }
        }
    }
    pthread_mutex_unlock(&q->lock);
    return item;
}

struct yqueue_item *yqueue_pop(struct yqueue *q, int fd, int *last)
{
    if (!q) {
        printf("invalid parament!\n");
        return NULL;
    }
    char notify;
#if 1
    if (read(fd, &notify, sizeof(notify)) != 1) {
        printf("read pipe fd = %d failed: %s\n", fd, strerror(errno));
    }
#endif
    if (list_empty(&q->head)) {
        return NULL;
    }
    struct yqueue_item *item = NULL;
    pthread_mutex_lock(&q->lock);
    item = list_first_entry_or_null(&q->head, struct yqueue_item, entry);
    if (item) {
        atomic_dec(&item->ref);
        if (!atomic_get(&item->ref)) {
            *last = 1;
            list_del(&item->entry);
            --(q->depth);
#if 0
            if (read(fd, &notify, sizeof(notify)) != 1) {
                printf("read pipe fd = %d failed: %s\n", fd, strerror(errno));
            }
#endif
        }
        //printf("pop item= %p, ref= %d\n", item, atomic_get(&item->ref));
    }
    pthread_mutex_unlock(&q->lock);
    return item;
}

int yqueue_push(struct yqueue *q, struct yqueue_item *item)
{
    if (!q || !item) {
        return -1;
    }
    char notify = '1';
    struct pipe_fds *pipe_item, *next;
    pthread_mutex_lock(&q->lock);
    atomic_set(&item->ref, q->ref_cnt);
    //printf("push item= %p, ref= %d\n", item, atomic_get(&item->ref));
    list_add_tail(&item->entry, &q->head);
    ++(q->depth);

    list_for_each_entry_safe(pipe_item, next, &q->pipe_list, entry) {
        if (write(pipe_item->wfd, &notify, sizeof(notify)) != 1) {
            printf("write pipe failed: %s\n", strerror(errno));
        }
    }
    pthread_mutex_unlock(&q->lock);
    if (q->depth > q->max_depth) {
        printf("queue depth reach max depth %d\n", q->depth);
        //yqueue_pop_absolute(q);
    }
    return 0;
}
struct yqueue *yqueue_create()
{
    struct yqueue *q = (struct yqueue *)calloc(1, sizeof(struct yqueue));
    if (!q) {
        return NULL;
    }
    INIT_LIST_HEAD(&q->head);
    INIT_LIST_HEAD(&q->pipe_list);
    pthread_mutex_init(&q->lock, NULL);

    q->ref_cnt = 0;
    q->depth = 0;
    q->max_depth = YQUEUE_MAX_DEPTH;
    //yqueue_add_ref(q);
    q->media_info = calloc(1, sizeof(&q->media_info));

    return q;
}

void yqueue_free(struct yqueue *q)
{
    struct yqueue_item *item, *next;
    struct pipe_fds *pipe_item, *pipe_next;
    if (!q) {
        return;
    }

    list_for_each_entry_safe(item, next, &q->head, entry) {
        list_del(&item->entry);
        yqueue_item_free(item);
    }
    pthread_mutex_destroy(&q->lock);
    list_for_each_entry_safe(pipe_item, pipe_next, &q->pipe_list, entry) {
        close(pipe_item->rfd);
        close(pipe_item->wfd);
    }

    free(q->media_info);
    free(q);
    q = NULL;
}
