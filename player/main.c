#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <event.h>

#include "yqueue.h"
#include "protocol.h"
#include "codec.h"
#include "display.h"
#include "agent.h"


typedef struct player {
    struct agent_ctx *downstream_agent;
    struct agent_ctx *vdecode_agent;
    struct agent_ctx *display_agent;
} player_t;

static struct player *player_instance = NULL;

struct player *player_init()
{
    player_t *p = (player_t *)calloc(1, sizeof(player_t));
    if (!p)
        return NULL;

    struct yqueue *q1 = yqueue_create();
    struct yqueue *q2 = yqueue_create();

    p->downstream_agent = agent_create("downstream", NULL, q1);
    p->vdecode_agent = agent_create("vdecode", q1, q2);
    p->display_agent = agent_create("display", q2, NULL);
    return p;
}

void player_dispatch(struct player *p)
{
    agent_dispatch(p->downstream_agent, 0);
    agent_dispatch(p->vdecode_agent, 0);
    agent_dispatch(p->display_agent, 0);
    while (1) {
        sleep(2);
    }
}

static void sigterm_handler(int sig)
{
    agent_destroy(player_instance->downstream_agent);
    agent_destroy(player_instance->vdecode_agent);
    agent_destroy(player_instance->display_agent);
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, sigterm_handler);

    protocol_register_all();
    codec_register_all();
    display_register_all();
    agent_register_all();

    player_instance = player_init();
    if (!player_instance) {
        return -1;
    }

    player_dispatch(player_instance);
    return 0;
}
