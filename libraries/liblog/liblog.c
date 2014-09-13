#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>

#include "liblog.h"

static char g_log_filename[256];
static FILE *g_log_fp = NULL;
static int g_log_level = 0;
static pthread_mutex_t g_log_lock;

static char *g_log_level_str[] = {
    "EMERG",
    "ALERT",
    "CRIT",
    "ERR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG",
    NULL
};

static void log_time(char *str, int flag_name)
{
    struct tm *p;
    long ts;
    int year, month, day, hour, min, sec;

    ts = time(NULL);
    p = localtime(&ts);
    year = p->tm_year + 1900;
    month = p->tm_mon + 1;
    day = p->tm_mday;
    hour = p->tm_hour;
    min = p->tm_min;
    sec = p->tm_sec;
    if (flag_name == 1)
        sprintf(str, "%4d_%02d_%02d_%02d_%02d_%02d.log", year, month, day, hour, min, sec);
    else
        sprintf(str, "%4d-%02d-%02d %02d:%02d:%02d ", year, month, day, hour, min, sec);
}

static void log_print_syslog(int level, const char *format, ...)
{
    if (level > g_log_level)
        return;
    va_list ap;
    va_start(ap, format);
    syslog(level, "%s: ", g_log_level_str[level]);
    vsyslog(level, format, ap);
    va_end(ap);
}

static void log_print_stderr(int level, const char *format, ...)
{
    if (level > g_log_level)
        return;
    char str_time[32] = {0};
    va_list ap;
    va_start(ap, format);
    log_time(str_time, 0);
    fputs(str_time, stderr);
    fprintf(stderr, "%s: ", g_log_level_str[level]);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

static void log_print_file(int level, const char *format, ...)
{
    if (level > g_log_level)
        return;
    pthread_mutex_lock(&g_log_lock);
    char str_time[32] = {0};
    va_list ap;
    g_log_fp = fopen(g_log_filename, "a+");
    if (g_log_fp == NULL) {
        fprintf(stderr, "fopen %s failed: %s\n",
                g_log_filename, strerror(errno));
        goto err;
    }

    va_start(ap, format);
    log_time(str_time, 0);
    fputs(str_time, g_log_fp);
    fprintf(g_log_fp, "%s: ", g_log_level_str[level]);
    vfprintf(g_log_fp, format, ap);
    va_end(ap);

err:
    fclose(g_log_fp);
    pthread_mutex_unlock(&g_log_lock);
}

int log_init(int type, int level, const char *ident)
{
    memset(g_log_filename, 0, sizeof(g_log_filename));
    g_log_fp = NULL;
    g_log_level = level;

    switch (type) {
    case LOG_STDERR:
        g_log_fp = stderr;
        g_log_print_func = &log_print_stderr;
        break;
    case LOG_FILE:
        if (ident == NULL) {
            log_time(g_log_filename, 1);
        } else {
            strncpy(g_log_filename, ident, sizeof(g_log_filename));
        }
        g_log_fp = fopen(g_log_filename, "a+");
        if (g_log_fp == NULL) {
            fprintf(stderr, "fopen %s failed: %s\n", g_log_filename, strerror(errno));
            return -1;
        }
        fprintf(stderr, "name = %s\n", g_log_filename);
        g_log_print_func = &log_print_file;
        fclose(g_log_fp);
        break;
    case LOG_RSYSLOG:
        openlog(ident, LOG_CONS | LOG_PID, level);
        g_log_print_func = &log_print_syslog;
        break;
    default:
        fprintf(stderr, "unsupport log type!\n");
        return -1;
        break;
    }

    pthread_mutex_init(&g_log_lock, NULL);
    return 0;
}
