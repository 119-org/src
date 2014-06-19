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

// url = rtsp://192.168.1.1:8554/1.264
// name = rtsp
// split = 192.168.1.1:8554/1.264
struct url {
    char name[32];
    char *split;
};

struct url *parse_url(const char *input, int len)
{
    struct url *u = (struct url *)calloc(1, sizeof(struct url));
    char *p;
    do {
        p = strchr(input, ':');
    } while (1);

}

struct source_ctx *source_init(const char *input)
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
