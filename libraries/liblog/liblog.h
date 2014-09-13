#ifndef _LIBLOG_H_
#define _LIBLOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* from /usr/include/sys/syslog.h */
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */

#define	LOG_PRIMASK	0x07	/* mask to extract priority part (internal) */
				/* extract priority */
#define	LOG_NO_PREFIX			(1<<6)

typedef enum {
    LOG_STDERR = 0,
    LOG_FILE,
    LOG_RSYSLOG,
} log_type_t;

void (*g_log_print_func)(int level, const char *format, ...);

int log_init(int type, int level, const char *ident);
#define log_print(level, format...)\
    do {\
        g_log_print_func(level, ##format);\
    } while (0);

#ifdef __cplusplus
}
#endif
#endif
