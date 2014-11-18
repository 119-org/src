#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"
#include "debug.h"


int parse_url(struct url *u, const char *input)
{
    char *p;
    int hlen, blen;
    char *url_tag = "://";

    p = strstr(input, url_tag);
    if (!p) {
        printf("input is not url format\n");
        return -1;
    }
    hlen = p - input;
    strncpy(u->head, input, hlen);
    p += strlen(url_tag);
    blen = input + strlen(input) - p;
    strncpy(u->body, p, blen);
    return 0;
}

void dump_buffer(void *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (i % 8 == 0)
            printf("\n:\t");
        printf("%02x ", (*((char *)buf + i)) & 0xff);
    }
}


