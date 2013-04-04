
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "libthreadpool.h"

static void *tp_thread(void *arg)
{
    if (arg == NULL) {
        printf("%s parament invalid!\n", __func__);
        return NULL;
    }
    threadpool_t *tp = (threadpool_t *)arg;
    thread_task_t *task;

    while (1) {
        pthread_mutex_lock(&tp->lock);
        while (tp->task_num == 0 && !(tp->shutdown)) {
            pthread_cond_wait(&(tp->notify), &(tp->lock));

        }
        if (tp->shutdown) {
            break;
        }
        task = tp->task_head;
        tp->task_head = tp->task_head->next;
        tp->task_tail = tp->task_tail->next;
        tp->task_num--;

        pthread_mutex_unlock(&tp->lock);
        if (task->func == NULL) {
            continue;
        }
        task->func(task->arg);
    }
    pthread_mutex_unlock(&tp->lock);
    pthread_exit(NULL);
    return NULL;
}

static threadpool_t *tp_create(uint32_t cnt)
{
    int i;
    threadpool_t *tp;

    tp = calloc(1, sizeof(threadpool_t));
    if (tp == NULL) {
        printf("calloc failed\n");
        return NULL;
    }
    tp->thread_max = cnt;
    tp->thread_num = 0;
    tp->task_num = 0;
    tp->shutdown = 0;
    tp->task_head = (thread_task_t *)calloc(1, sizeof(thread_task_t));
    tp->task_head->next = tp->task_head;
    tp->task_tail = tp->task_head;
    tp->task_tail->next = tp->task_head;
    tp->tid = (pthread_t *)calloc(cnt, sizeof(pthread_t));

    if ((pthread_mutex_init(&tp->lock, NULL) != 0) ||
        (pthread_cond_init(&tp->notify, NULL) != 0)) {
        printf("pthread mutex or cond error!\n");
        return NULL;
    }

    for (i = 0; i < cnt; i++) {
        if (pthread_create(&(tp->tid[i]), NULL, tp_thread, (void *)tp) != 0) {
            printf("pthread_create failed, errno:%d, error:%s\n", errno, strerror(errno));
            return NULL;
        }
    }
    return tp;
}

static int tp_expand(threadpool_t * tp, uint32_t size)
{
    int i;
    uint32_t base = tp->thread_max;

    tp->thread_max += size;
    tp->tid = (pthread_t *)realloc(tp->tid, tp->thread_max * sizeof(pthread_t));
    memset(tp->tid + base, 0, size);
    for (i = 0; i < size; i++) {
        if (pthread_create(&(tp->tid[base + i]), NULL, tp_thread, (void *)tp) != 0) {
            printf("pthread_create failed, errno:%d, error:%s\n", errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

int tp_add(threadpool_t * tp, void (*func)(void *), void *arg)
{
    if (tp == NULL || func == NULL) {
        printf("%s parments invalid!\n", __func__);
        return -1;
    }

    thread_task_t *task = calloc(1, sizeof(thread_task_t));
    if (task == NULL) {
        printf("calloc failed\n");
        return -1;
    }

    if (pthread_mutex_lock(&(tp->lock)) != 0) {
        return -1;
    }

    tp->thread_num++;
    if (tp->thread_num > tp->thread_max) {
        tp_expand(tp, 10);
    }
    task->func = func;
    task->arg = arg;
    task->next = tp->task_head;
    tp->task_tail->next = task;
    tp->task_head = task;
    tp->task_num++;

    pthread_cond_signal(&(tp->notify));//only notify one thread
    pthread_mutex_unlock(&(tp->lock));

    return 0;
}

static int tp_destory(threadpool_t * tp)
{
    int i;
    void *val;
    thread_task_t *p;

    if (tp->shutdown) {
        return 0;
    }
    tp->shutdown = 1;

    pthread_mutex_lock(&tp->lock);
    pthread_cond_broadcast(&tp->notify);
    pthread_mutex_unlock(&tp->lock);

    for (i = 0; i < tp->thread_max; i++) {
        if (tp->tid[i] != 0) {
            if (0 != pthread_join(tp->tid[i], &val)) {
                printf("pthread_join error!\n");
            }
        }
    }
    free(tp->tid);

    while (tp->task_head != tp->task_tail) {
        p = tp->task_head;
        tp->task_head = tp->task_head->next;
        free(p);
    }

    pthread_mutex_destroy(&tp->lock);
    pthread_cond_destroy(&tp->notify);

    free(tp);
    return 0;
}

threadpool_t *tp_init()
{
    return tp_create(MAX_THREAD);
}

int tp_uninit(threadpool_t *tp)
{
    return tp_destory(tp);
}
