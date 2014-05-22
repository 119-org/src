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

struct pollop {
	int event_count;		/* Highest number alloc */
	int nfds;			/* Highest number used */
	int realloc_copy;		/* True iff we must realloc
					 * event_set_copy */
	struct pollfd *event_set;
	struct pollfd *event_set_copy;
};


static void *poll_init();
static int poll_add();
static int poll_del();
static int poll_dispatch();

