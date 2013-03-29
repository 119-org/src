
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libmempool.h"

#define ALIGN_BYTES	8
#define MAX_BYTES	1024
#define GROW_SIZE	100

/*
 * free_list[0]: 8 bytes
 * free_list[1]: 16 bytes
 * free_list[n]: (n+1) * 8 bytes
 * free_list[127]: 1024 bytes
 * large_list *: > 1024 bytes
 */

mempool_t *mp_create(uint32_t align_bytes, uint32_t max_align, uint32_t grow_size)
{
    if (align_bytes < 4 || max_align < align_bytes) {
        printf("%s paraments invalid!\n", __func__);
        return NULL;
    }
    if (align_bytes & (align_bytes - 1)) {
        printf("align_bytes must be 2^n\n");
        return NULL;
    }

    int i;
    int free_list_num = max_align / align_bytes + !!(max_align % align_bytes);
    if (free_list_num > 1024 * 1024) {
        printf("warning: free_list num is too big!\n");
    }
    mempool_t *mp = (mempool_t *)malloc(sizeof(mempool_t));
    if (mp == NULL) {
        printf("malloc failed!\n");
        return NULL;
    }
    memset(mp, 0, sizeof(mempool_t));
    mp->align_bytes = align_bytes;
    mp->max_align = max_align;
    mp->grow_size = grow_size;
    mp->free_list = (mp_block_list_t *)malloc(free_list_num * sizeof(mp_block_list_t));
    if (mp->free_list == NULL) {
        printf("malloc failed!\n");
        return NULL;
    }
    for (i = 0; i < free_list_num; i++) {
        mp_block_list_t *list = mp->free_list + i;
        mp_block_t *blk = (mp_block_t *)malloc(sizeof(mp_block_t));
        if (blk == NULL) {
            printf("malloc failed!\n");
            return NULL;
        }
        blk->size = align_bytes * (i + 1);
        blk->blk_num = 0;
        blk->free_blk = 0;
        blk->data = NULL;
        blk->next = blk;
        list->blk_size = align_bytes * (i + 1);
        list->blk_head = blk;
        list->blk_tail = blk;
    }

    mp->large_list = (mp_block_large_list_t *)malloc(sizeof(mp_block_large_list_t));
    if (mp->large_list == NULL) {
        printf("malloc failed!\n");
        return NULL;
    }
    mp_block_large_t *large = (mp_block_large_t *)malloc(sizeof(mp_block_large_t));
    if (large == NULL) {
        printf("malloc failed!\n");
        return NULL;
    }
    large->size = 0;
    large->data = NULL;
    large->next = large;
    mp->large_list->blk_num = 0;
    mp->large_list->blk_head = large;
    mp->large_list->blk_tail = large;

    return mp;
}

int mp_add_chunk(mp_block_list_t *blk_list, uint32_t width, uint32_t height)
{
    int i;
    void *data = NULL;
    uint32_t data_size = width + sizeof(uint32_t);//data unit size
    uint32_t chunk_size = sizeof(mp_block_t) + height * data_size;
    mp_block_t *chunk = (mp_block_t *)malloc(chunk_size);
    if (chunk == NULL) {
        printf("malloc failed!\n");
        return -1;
    }
    memset(chunk, 0, chunk_size);
    //init chunk
    chunk->size = width;//XXX: remeber size of block, use this value when free
    chunk->blk_num = height;
    chunk->free_blk = height;
    chunk->data = (void *)((uint32_t)chunk + sizeof(mp_block_t));
    //init data of chunk
    data = (void *)((uint32_t)chunk + sizeof(mp_block_t));
    for (i = 0; i < height - 1; i++) {//0 ~ (last-1) block
        //data->data: data + sizeof(uint32_t)
        //data->next: data + 0
        *(uint32_t *)data = (uint32_t)((uint32_t)data + data_size);//data->next = data + data_size
        data = (void *)((uint32_t)data + data_size);//data = data->next
    }
    *(uint32_t *)data = 0;//XXX last block point to zero or something else?
    //XXX: insert new chunk to head of block list
    chunk->next = blk_list->blk_head;
    blk_list->blk_head = chunk;
    blk_list->blk_tail->next = chunk;
    return 0;
}
int mp_add_chunk_large(mp_block_large_list_t *blk_list, uint32_t size)
{
    void *data = NULL;
    uint32_t chunk_size = size + sizeof(uint32_t) + sizeof(mp_block_large_t);
    mp_block_large_t *chunk = (mp_block_large_t *)malloc(chunk_size);
    if (chunk == NULL) {
        printf("malloc failed!\n");
        return -1;
    }
    memset(chunk, 0, chunk_size);
    data = (void *)((uint32_t)chunk + sizeof(mp_block_large_t));
    *(uint32_t *)data = 0;//new large data->next = NULL
    chunk->size = size;
    chunk->data = (void *)((uint32_t)chunk + sizeof(mp_block_large_t));
    chunk->next = blk_list->blk_head;
    blk_list->blk_head = chunk;
    blk_list->blk_tail->next = chunk;
    return 0;
}

void mp_del_chunk(mp_block_list_t *list)
{
    if (list == NULL) {
        printf("align chunk is empty!\n");
        return;
    }
    mp_block_t *p = list->blk_head;
    list->blk_head = p->next;
    list->blk_tail->next = p->next;
    free((void *)p);
}

void mp_del_chunk_large(mp_block_large_list_t *list)
{
    if (list == NULL) {
        printf("large chunk is empty!\n");
        return;
    }
    mp_block_large_t *p = list->blk_head;
    list->blk_head = p->next;
    list->blk_tail->next = p->next;
    free((void *)p);
}

static inline void *mp_alloc_align(mempool_t *mp, size_t size)
{
    if (mp == NULL || size == 0) {
        printf("%s paraments invalid!\n", __func__);
        return NULL;
    }

    void *data = NULL;
    int index = size / mp->align_bytes - 1;
    mp_block_list_t *blk_list = mp->free_list + index;
    mp_block_t *p;

    p = blk_list->blk_head;
    do {
        if (p->next == blk_list->blk_head) {
            if (-1 == mp_add_chunk(blk_list, size, mp->grow_size)) {
                printf("mp_add_chunk failed!\n");
                return NULL;
            }
            p = blk_list->blk_head;
        }
        if (p->free_blk == 0) {
            p = p->next;
            continue;
        }
        data = p->data;	//chose first data
//        p->data = (*(uint32_t *)(*(uint32_t *)data));	//change blk_head->data point to data->next
        p->data = *(uint32_t *)data;	//change blk_head->data point to data->next
        *(uint32_t *)data = (uint32_t)p;	//data which is used, point to blk_head
        data = (void *)(data + sizeof(uint32_t));	//move data to data->data
        p->free_blk--;
        break;
    } while (data == NULL);

    return data;
}

static inline void *mp_alloc_large(mempool_t *mp, size_t size)
{
    if (mp == NULL || size == 0) {
        printf("%s paraments invalid!\n", __func__);
        return NULL;
    }
    void *data = NULL;
    mp_block_large_list_t *blk_list = mp->large_list;
    mp_block_large_t *p = blk_list->blk_head;

    for (p = blk_list->blk_head; p->next != blk_list->blk_tail; p = p->next) {
        if (!(*(uint32_t *)(p->data) == NULL && p->size == size)) {//find same size empty block
            continue;
        }
        data = p->data;//choose data
        *(uint32_t *)data = (uint32_t)p;	//data which is used, point to blk_head
        data = (void *)(data + sizeof(uint32_t));//move data to data->data
        return data;
    }

    do {
        if (-1 == mp_add_chunk_large(blk_list, size)) {
            printf("mp_add_chunk_large failed!\n");
            return NULL;
        }
        p = blk_list->blk_head;
        data = p->data;//choose data
        *(uint32_t *)data = (uint32_t)p;	//data which is used, point to blk_head
        data = (void *)(data + sizeof(uint32_t));//move data to data->data
        blk_list->blk_num++;
        break;
    } while (data == NULL);
    return data;
}

void *mp_alloc(mempool_t *mp, size_t size)
{
    if (mp == NULL || size == 0) {
        if (size != 0)
            printf("%s paraments invalid!\n", __func__);
        return NULL;
    }
    size_t alloc_size = (size + mp->align_bytes - 1) & ~(mp->align_bytes - 1);//align, e.g. 17 -> 24

    if (alloc_size > mp->max_align) {
        return mp_alloc_large(mp, alloc_size);
    } else {
        return mp_alloc_align(mp, alloc_size);
    }
}

void mp_free_align(mempool_t *mp, void *data)
{
    if (mp == NULL || data == NULL) {
        printf("%s paraments invalid!\n", __func__);
        return;
    }
    mp_block_t *blk = (mp_block_t *)(*(uint32_t *)data);//get blk_head from used_data->next
    *(uint32_t *)data = (uint32_t)(blk->data);//data->next to blk_head->data
    blk->data = data;//insert data to blk_head->data
    blk->free_blk++;
}

void mp_free_large(mempool_t *mp, void *data)
{
    if (mp == NULL || data == NULL) {
        printf("%s paraments invalid!\n", __func__);
        return;
    }
    *(uint32_t *)data = 0;
    mp->large_list->blk_num--;
}

void mp_free(mempool_t *mp, void *data)
{
    if (mp == NULL || data == NULL) {
        printf("%s paraments invalid!\n", __func__);
        return;
    }

    data = (void *)(data - sizeof(uint32_t));//data->ptr
    uint32_t size = *(uint32_t *)(*(uint32_t *)data);//blk_head->size

    if (size > mp->max_align) {
        return mp_free_large(mp, data);
    } else {
        return mp_free_align(mp, data);
    }
}

void mp_dbg_show(mempool_t *mp)
{
    int i;
    if (mp == NULL) {
        printf("mempool is empty!\n");
        return;
    }
    for (i = 0; i < mp->max_align / mp->align_bytes; i++) {
        printf("align block list[%d]:\n", i);
    }

}

int mp_destory(mempool_t *mp)
{
    if (mp == NULL) {
        printf("mempool is empty!\n");
        return -1;
    }
    int i;
    int index = mp->max_align / mp->align_bytes + !!(mp->max_align % mp->align_bytes);
    mp_block_list_t *blk_list;
    mp_block_large_list_t *large_list;

    for (i = 0; i < index; i++) {
        blk_list = mp->free_list + i;
        while (blk_list != NULL) {
            if (blk_list->blk_tail == blk_list->blk_head) {
                blk_list = NULL;
                continue;
            }
            mp_del_chunk(blk_list);
        }
    }

    large_list = mp->large_list;
    while (large_list != NULL) {
        if (large_list->blk_tail == large_list->blk_head) {
            large_list = NULL;
            continue;
        }
        mp_del_chunk_large(large_list);
    }

    free(mp);
    return 0;
}

mempool_t *mp_init()
{
    return mp_create(ALIGN_BYTES, MAX_BYTES, GROW_SIZE);
}

int mp_uninit(mempool_t *p)
{
    return mp_destory(p);
}
