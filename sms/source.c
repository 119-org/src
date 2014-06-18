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

#include "libskt/libskt.h"
#include "sms.h"

struct source {

};

struct sms g_sms;

int source_init()
{
    return 0;
}



int main(int argc, char **argv)
{
    source_init();
    sms_loop();

    return 0;
}
