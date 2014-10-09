#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HAVE_ATOMIC_GCC 1

typedef struct {
    int counter;
} atomic_t;


#ifdef HAVE_ATOMIC_GCC
void atomic_inc(atomic_t *v, int i)
{
    __sync_add_and_fetch(&v->counter, i);
}

void atomic_dec(atomic_t *v, int i)
{
    __sync_add_and_fetch(&v->counter, 0 - i);
}
#else
void atomic_inc(atomic_t *v, int i)
{
#if (defined __x86_64__ || __i386__)
    __asm__ volatile (
            "lock; addl %1, %0\n"
            : "+m" (v->counter)
            : "ir" (i)
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
            : "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
            : "r" (&v->counter), "Ir" (i)
            : "cc");
#else
#error atomic_ops not implemented in this arm, add -march=armv7-a to cflags
#endif

}

void atomic_dec(atomic_t *v, int i)
{
#if (defined __x86_64__ || __i386__)
    __asm__ volatile (
            "lock; subl %1, %0\n"
            : "+m" (v->counter)
            : "ir" (i)
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
            : "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
            : "r" (&v->counter), "Ir" (i)
            : "cc");
#else
#error atomic_ops not implemented in this arm, add -march=armv7-a to cflags
#endif
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
#endif
