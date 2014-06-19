#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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


#include "libskt/libskt.h"
#include "libmempool/libmempool.h"
#include "sms.h"

struct sms		g_sms;
struct mempool		*g_mp;
struct threadpool	*g_tp;


int sms_init()
{
    return 0;
}

int sms_loop()
{
    while (1) {
        sleep(1);
    }

    return 0;
}

int main(int argc, char **argv)
{
    sms_init();
    sms_loop();

    return 0;
}
