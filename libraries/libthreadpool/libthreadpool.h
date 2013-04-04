#ifndef _LIBTHREADPOOL_H_
#define _LIBTHREADPOOL_H_

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THREAD	100

typedef struct thread_task {
    void (*func)(void *);
    void *arg;
    struct thread_task *next;
} thread_task_t;

typedef struct threadpool {
    uint32_t thread_max;
    uint32_t thread_num;
    uint32_t task_num;
    int shutdown;
    pthread_t *tid;
    thread_task_t *task_head;
    thread_task_t *task_tail;
    pthread_cond_t notify;
    pthread_mutex_t lock;
} threadpool_t;

extern threadpool_t *tp_init();
extern int tp_uninit(threadpool_t *tp);
extern int tp_add(threadpool_t *tp, void (*func)(void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif
