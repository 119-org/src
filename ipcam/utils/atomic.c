#include "atomic.h"

/*
 * define GCC_SUPPORT_ATOMIC if gcc version > 4.1.2 and less than < 4.7
 */
#define GCC_SUPPORT_ATOMIC 1
#ifdef GCC_SUPPORT_ATOMIC
int atomic_get(struct atomic *v)
{
    __sync_synchronize();
    return v->value;
}
void atomic_set(struct atomic *v, int num)
{
    v->value = num;
    __sync_synchronize();
}
void atomic_add(struct atomic *v, int num)
{
    __sync_add_and_fetch(&v->value, num);
}

void atomic_sub(struct atomic *v, int num)
{
    __sync_sub_and_fetch(&v->value, num);
}
#else
void atomic_add(struct atomic *v, int num)
{
#if (defined __x86_64__ || __i386__)
    __asm__ volatile (
            "lock; addl %1, %0\n"
            : "+m" (v->value)
            : "ir" (num)
            : "cc", "memory");
#elif (defined  __ARM_ARCH_7A__)
    /* work if add -march=armv7-a */
    unsigned long tmp;
    int result;
    __asm__ volatile (
            "  dmb sy\n"
            "1: ldrex   %0, [%3]\n"
            "   add %0, %0, %4\n"
            "   strex   %1, %0, [%3]\n"
            "   teq %1, #0\n"
            "   bne 1b"
            : "=&r" (result), "=&r" (tmp), "+Qo" (v->value)
            : "r" (&v->value), "Ir" (num)
            : "cc");
#else
#error atomic_ops not implemented in this arm, add -march=armv7-a to cflags
#endif

}

void atomic_sub(struct atomic *v, int num)
{
#if (defined __x86_64__ || __i386__)
    __asm__ volatile (
            "lock; subl %1, %0\n"
            : "+m" (v->value)
            : "ir" (num)
            : "cc", "memory");
#elif (defined  __ARM_ARCH_7A__)
    unsigned long tmp;
    int result;
    /* work if add -march=armv7-a */
    __asm__ volatile (
            "  dmb sy\n"
            "1: ldrex   %0, [%3]\n"
            "   sub %0, %0, %4\n"
            "   strex   %1, %0, [%3]\n"
            "   teq %1, #0\n"
            "   bne 1b"
            : "=&r" (result), "=&r" (tmp), "+Qo" (v->value)
            : "r" (&v->value), "Ir" (num)
            : "cc");
#else
#error atomic_ops not implemented in this arm, add -march=armv7-a to cflags
#endif
}
#endif

void atomic_inc(struct atomic *v)
{
    return atomic_add(v, 1);
}
void atomic_dec(struct atomic *v)
{
    return atomic_sub(v, 1);
}
