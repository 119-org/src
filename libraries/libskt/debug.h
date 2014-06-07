#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#define print_func_line(flag, arg...)\
    do {\
        fprintf(stderr, "%s: %s:%d: ", flag, __func__, __LINE__);\
        fprintf(stderr, ##arg);\
    } while (0);

#define dbg(arg...) \
    print_func_line("DBG", ##arg);

#define err(arg...) \
    print_func_line("ERR", ##arg);

#define info(arg...) \
    print_func_line("INFO", ##arg);

#endif
