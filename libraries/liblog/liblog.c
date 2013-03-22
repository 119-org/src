
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

#include "liblog.h"

FILE *log_fp;
static char log_name[32];
static pthread_mutex_t mutex_lock;

static char *LOG_STR[MAX_LEVEL] = {
    "ERROR:",
    "WARN:",
    "DEBUG:",
    "INFO:"
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

void log_init(char *log_out)
{
    if (!strcmp(log_out, "stdout")) {
        log_fp = stdout;
    } else if (!strcmp(log_out, "stderr")) {
        log_fp = stderr;
    } else if (!strcmp(log_out, "file")) {
        log_time(log_name, 1);
        log_fp = fopen(log_name, "w+");
        printf("name = %s\n", log_name);
        fclose(log_fp);
    }
    pthread_mutex_init(&mutex_lock, NULL);
}

void log(log_level_t flag, const char *str, ...)
{
    pthread_mutex_lock(&mutex_lock);

    if (log_fp != stdout && log_fp != stderr) {
        log_fp = fopen(log_name, "a+");
    }

    va_list args;
    char str_time[32] = {0};

    log_time(str_time, 0);
    va_start(args, str);
    fputs(str_time, log_fp);
    flag = (flag > MAX_LEVEL) ? INFO : flag;
    fputs(LOG_STR[flag], log_fp);
    fputc(' ', log_fp);
    vfprintf(log_fp, str, args);
    fputc('\n', log_fp);
    va_end(args);

    if (log_fp != stdout && log_fp != stderr) {
        fclose(log_fp);
    }
    pthread_mutex_unlock(&mutex_lock);
}

