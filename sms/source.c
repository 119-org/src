#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include "debug.h"
#include "source.h"


struct source_ctx *source_init()
{
    struct source_ctx *sc = (struct source_ctx *)calloc(1, sizeof(struct source_ctx));
    if (!sc) {
        err("malloc source context failed!\n");
        return NULL;
    }

    return sc;
}


void source_deinit(struct source_ctx *sc)
{
    if (sc)
        free(sc);
    return;
}
