
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libmempool.h"

#include "../libmisc/libmisc.h"
#include "../libmisc/libmisc.c"

#define MEMPOOL 1

void alloc_align_test(mempool_t *mp, int cnt)
{
    struct timeval tv_start, tv_end;
    float gap = 0.0;
    uint32_t i, rand, size;

    gettimeofday(&tv_start, NULL);
    void *mem[1000];
    memset(mem, 0, sizeof(mem));
    srandom(time(NULL));
    for (i = 0; i < cnt; i++) {
        rand = random() % (sizeof(mem) / sizeof(void *));
        size = random() % (mp->max_align + 1);
        if (mem[rand] == NULL) {
#if (MEMPOOL)
            mem[rand] = mp_alloc(mp, size);
#else
            mem[rand] = malloc(size);
#endif
            if (mem[rand] == NULL) {
//                printf("mp_alloc size = %d failed!\n", rand);
                continue;
            }
        } else {
#if (MEMPOOL)
            mp_free(mp, mem[rand]);
#else
            free(mem[rand]);
#endif
            mem[rand] = NULL;
        }
    }
#if 0
        memset(mem[i], 0, i);
        for (uint32_t j = 0; j < i; j++) {
            *((char *)(mem[i]) + j) = j + 1;
        }
        for (j = 0; j < i; j++) {
            printf("mem[%d] = %04u\t", i, *((char *)(mem[i]) + j));
            if (j % 9 == 8) printf("\n");
        }
        printf("\n");
#endif
    for(i = 0; i < sizeof(mem) / sizeof(void *); i++) {
        if (mem[i] != NULL) {
#if (MEMPOOL)
            mp_free(mp, mem[i]);
#else
            free(mem[i]);
#endif
            mem[i] = NULL;
        }
    }
    gettimeofday(&tv_end, NULL);
    gap = tv_end.tv_sec - tv_start.tv_sec;
    gap += (float)(tv_end.tv_usec - tv_start.tv_usec) / (float)(1000000);
    printf("cost time: %.6f s.\n", gap);
}

void alloc_large_test(mempool_t *mp, int cnt)
{
    struct timeval tv_start, tv_end;
    float gap = 0.0;
    uint32_t i, rand, size;

    gettimeofday(&tv_start, NULL);
    void *mem[1000];
    memset(mem, 0, sizeof(mem));
    srandom(time(NULL));
    for (i = 0; i < cnt; i++) {
        rand = random() % (sizeof(mem) / sizeof(void *));
        size = random() % mp->max_align + mp->max_align + 1;
        if (mem[rand] == NULL) {
#if (MEMPOOL)
            mem[rand] = mp_alloc(mp, size);
#else
            mem[rand] = malloc(size);
#endif
            if (mem[rand] == NULL) {
                continue;
            }
        } else {
#if (MEMPOOL)
            mp_free(mp, mem[rand]);
#else
            free(mem[rand]);
#endif
            mem[rand] = NULL;
        }
    }
    for(i = 0; i < sizeof(mem) / sizeof(void *); i++) {
        if (mem[i] != NULL) {
#if (MEMPOOL)
            mp_free(mp, mem[i]);
#else
            free(mem[i]);
#endif
            mem[i] = NULL;
        }
    }
    gettimeofday(&tv_end, NULL);
    gap = tv_end.tv_sec - tv_start.tv_sec;
    gap += (float)(tv_end.tv_usec - tv_start.tv_usec) / (float)(1000000);
    printf("cost time: %.6f s.\n", gap);
}
void foo(mempool_t *mp)
{
    uint32_t i;
    void *tmp1, *tmp2, *tmp3;
    uint32_t len1 = 1617, len2 = 1942, len3 = 300;
    tmp1 = mp_alloc(mp, len1);
    tmp2 = mp_alloc(mp, len2);
    tmp3 = mp_alloc(mp, len3);
#if 0
    for (i = 0; i < len1; i++) {
        *((char *)tmp1 + i) = (char)i;
    }
    for (i = 0; i < len1; i++) {
        printf("tmp1[%d] = %04u\t", i, *((char *)tmp1 + i));
        if (i % 9 == 8) printf("\n");
    }
#endif
    mp_free(mp, tmp1);
    mp_free(mp, tmp2);
    mp_free(mp, tmp3);
}
int main(int argc, char **argv)
{
    mempool_t *mp = mp_init();
    if (mp == NULL) {
        printf("mp_create failed!\n");
    }
    alloc_align_test(mp, 1000 * 1000);
    alloc_large_test(mp, 1000 * 1000);

//    foo(mp);
    printf("mp_free success!\n");
    mp_destory(mp);
    return 0;
}
