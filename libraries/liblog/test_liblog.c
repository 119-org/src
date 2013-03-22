
#include <stdio.h>
#include <stdlib.h>
#include "liblog.h"

int main(int argc, char **argv)
{
    int i = 0;
    log_init("file");
    log(DEBUG, "%s, %d", "hello world", 1234);
    log(DEBUG, "");
    for (i = 0; i < 3; i++) {
    log(ERROR, "%s:%s:%d hello world", __FILE__, __func__, __LINE__);
    sleep(1);
    }

    return 0;
}
