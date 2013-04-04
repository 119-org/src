
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "libthreadpool.h"

void foo(void *arg)
{
    printf("thread %x: %d\n", pthread_self(), (int)arg);
    return;
}

int main(int argc, char **argv)
{
    int i;
    threadpool_t *pool;

    pool = tp_init();

    for (i = 0; i < 11; i++) {
        tp_add(pool, &foo, (void *)i);
    }
    usleep(10000);
    tp_uninit(pool);
    return 0;
}
