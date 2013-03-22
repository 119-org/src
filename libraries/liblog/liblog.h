#ifndef _LIBLOG_H_
#define _LIBLOG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ERROR = 0,
    WARN,
    DEBUG,
    INFO,
    MAX_LEVEL
}log_level_t;



extern void log_init(char *log_out);
extern void log(log_level_t flag, const char *str, ...);


#ifdef __cplusplus
}
#endif

#endif
