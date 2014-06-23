#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

enum debug_level {
    ERR,
    WARN,
    INFO,
    DBG,

    MAX = 10
};

static int verbose = 1;  // func:line:
static int level = MAX;

#define print_func_line(flag, arg...)\
    do {\
        if (verbose)\
            fprintf(stderr, "%s: %s:%d: ", flag, __func__, __LINE__);\
        else\
            fprintf(stderr, "%s: ", flag);\
        fprintf(stderr, ##arg);\
    } while (0);

#define err(arg...) \
    do { \
        if (level < ERR) break; \
        print_func_line("ERR", ##arg); \
    } while (0);

#define warn(arg...) \
    do { \
        if (level < WARN) break; \
        print_func_line("WARN", ##arg); \
    } while (0);

#define info(arg...) \
    do { \
        if (level < INFO) break; \
        print_func_line("INFO", ##arg); \
    } while (0);

#define dbg(arg...) \
    do { \
        if (level < DBG) break; \
        print_func_line("DBG", ##arg); \
    } while (0);

#endif
