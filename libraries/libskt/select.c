#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "event.h"

#define CALLOC(type, ptr) \
    ptr = (type)calloc(1, sizeof(type));

struct selectop {
    int event_fds;		/* Highest fd in fd set */
    int event_fdsz;
    int resize_out_sets;
    fd_set *event_readset_in;
    fd_set *event_writeset_in;
    fd_set *event_readset_out;
    fd_set *event_writeset_out;
};

static void *select_init()
{
    struct selectop *sop;
    sop = (struct selectop *)calloc(1, sizeof(struct selectop));
    if (!sop) {
        fprintf(stderr, "malloc selectop failed!\n");
        return NULL;
    }
    return sop;
}

static int select_add()
{

}

static int select_del()
{

}

static int select_dispatch()
{

}

const struct eventop selectops = {
	select_init,
	select_add,
	select_del,
	select_dispatch,
};

