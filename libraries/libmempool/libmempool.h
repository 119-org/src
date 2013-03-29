#ifndef _LIBMEMPOOL_H_
#define _LIBMEMPOOL_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mp_block {
    uint32_t size;	//data size, diff from large block, used to free
    uint32_t blk_num;	//blk num inside chunk
    uint32_t free_blk;	//blk left
    struct mp_block *next;
    void *data;
} mp_block_t;

typedef struct mp_block_large {
    uint32_t size;	//data size, diff from block
    struct mp_block_large *next;
    void *data;
} mp_block_large_t;

typedef struct mp_block_list {
    uint32_t blk_size;	//align size of block
    mp_block_t *blk_head;	//head of list
    mp_block_t *blk_tail;	//tail of list
} mp_block_list_t;

typedef struct mp_block_large_list {
    uint32_t blk_num;	//XXX: remove? blk num of large list
    mp_block_large_t *blk_head;	//head of list
    mp_block_large_t *blk_tail;	//tail of list
} mp_block_large_list_t;

typedef struct mempool {
    uint32_t align_bytes;
    uint32_t max_align;
    uint32_t grow_size;
    mp_block_list_t *free_list;
    mp_block_large_list_t *large_list;
} mempool_t;

extern mempool_t *mp_init();
extern int mp_uninit(mempool_t *mp);
extern void *mp_alloc(mempool_t *mp, size_t size);
extern void mp_free(mempool_t *mp, void *data);

	
#ifdef __cplusplus
}
#endif
#endif
