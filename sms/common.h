#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// url = rtsp://192.168.1.1:8554/1.264
// name = rtsp
// body = 192.168.1.1:8554/1.264
struct url {
    char head[32];
    char body[512];
};

int parse_url(struct url *u, const char *input);

struct frame {
    void *addr;
    int len;
    int index;
};


#ifdef __cplusplus
}
#endif
#endif
