#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <signal.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "event.h"

struct pollop {
	int event_count;		/* Highest number alloc */
	int nfds;			/* Highest number used */
	int realloc_copy;		/* True iff we must realloc
					 * event_set_copy */
	struct pollfd *event_set;
	struct pollfd *event_set_copy;
};


static void *poll_init()
{

}
static int poll_add(struct event_base *e, int fd, struct event_cbs *evcb)
{

    return 0;
}
static int poll_del(struct event_base *e, int fd, short events)
{

    return 0;
}
static int poll_dispatch(struct event_base *e, struct timeval *tv)
{

    return 0;
}

const struct event_ops pollops = {
	poll_init,
	poll_add,
	poll_del,
	poll_dispatch,
};

