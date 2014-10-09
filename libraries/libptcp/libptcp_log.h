#ifndef _LIBSKT_LOG_H_
#define _LIBSKT_LOG_H_

#include <stdio.h>
#include <string.h>

/*from /usr/include/sys/syslog.h */
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */

void ptcp_log(int level, const char *fmt, ...);


typedef struct log_str {
    int val;
    char *str;
} log_str_t;

struct log_str log_prefix[] = {
    {LOG_EMERG, "EMERG"},
    {LOG_ALERT, "ALERT"},
    {LOG_CRIT, "CRIT"},
    {LOG_ERR, "ERR"},
    {LOG_WARNING, "WARNING"},
    {LOG_NOTICE, "NOTICE"},
    {LOG_INFO, "INFO"},
    {LOG_DEBUG, "DEBUG"},
    {-1, NULL},
};

void (*ptcp_log_cb)(int level, const char* format, va_list vl) = NULL;

void ptcp_log_set(void (*cb)(int level, const char* format, va_list vl))
{
    ptcp_log_cb = cb;
}

void ptcp_vfprintf(int level, const char *fmt, va_list vl)
{
    char buf[1024];
    buf[0] = 0;
    snprintf(buf, sizeof(buf), "%s: ", log_prefix[level].str);
    vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, vl);
    fputs(buf, stderr);
}

void ptcp_vlog(int level, const char *fmt, va_list vl)
{
    if (ptcp_log_cb) {
        ptcp_log_cb(level, fmt, vl);
    } else {
        ptcp_vfprintf(level, fmt, vl);
    }
}

void ptcp_log(int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ptcp_vlog(level, fmt, vl);
    va_end(vl);
}
#endif
