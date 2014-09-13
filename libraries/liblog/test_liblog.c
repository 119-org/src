#include <stdio.h>
#include <stdlib.h>
#include "liblog.h"

void test_rsyslog()
{
    int i;
    log_init(LOG_RSYSLOG, LOG_INFO, NULL);
    for (i = 0; i < 1000; i++) {
        log_print(LOG_DEBUG, "debug msg\n");
        log_print(LOG_INFO, "debug msg\n");
    }
}

void test_file_name()
{
    int i;
    log_init(LOG_FILE, LOG_INFO, "foo.log");
    for (i = 0; i < 1000; i++) {
        log_print(LOG_INFO, "%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        log_print(LOG_INFO, "debug msg\n");
    }
}

void test_file_noname()
{
    int i;
    log_init(LOG_FILE, LOG_INFO, NULL);
    for (i = 0; i < 1000; i++) {
        log_print(LOG_INFO, "%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        log_print(LOG_INFO, "debug msg\n");
    }
}

void test()
{
    int i;
    log_init(LOG_STDERR, LOG_INFO, NULL);
    for (i = 0; i < 10000; i++) {
        log_print(LOG_DEBUG, "%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        log_print(LOG_INFO, "%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        log_print(LOG_NOTICE, "%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
    }
}

int main(int argc, char **argv)
{
    test();
    test_file_name();
    test_file_noname();
    test_rsyslog();
    return 0;
}
