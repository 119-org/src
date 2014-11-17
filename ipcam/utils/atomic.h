#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct atomic {
    volatile int32_t value;
};

int atomic_get(struct atomic *vaule);
void atomic_set(struct atomic *vaule, int num);

void atomic_inc(struct atomic *value);
void atomic_dec(struct atomic *value);

void atomic_add(struct atomic *value, int num);
void atomic_sub(struct atomic *value, int num);

#ifdef __cplusplus
}
#endif
#endif
