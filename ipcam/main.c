#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include "device.h"
#include "protocol.h"
#include "display.h"
#include "codec.h"
#include "buffer.h"
#include "yqueue.h"
#include "agent.h"

typedef struct ipcam {
    struct agent_ctx *videocap_agent;
    struct agent_ctx *display_agent;
    struct agent_ctx *x264enc_agent;
    struct agent_ctx *upstream_agent;
} ipcam_t;

static struct ipcam *ipcam_instance = NULL;

struct ipcam *ipcam_init()
{
    ipcam_t *ipc = (ipcam_t *)calloc(1, sizeof(ipcam_t));
    if (!ipc)
        return NULL;

    struct yqueue *q1 = yqueue_create();
    struct yqueue *q2 = yqueue_create();

    ipc->videocap_agent = agent_create("videocap", NULL, q1);
    ipc->display_agent = agent_create("display", q1, NULL);
    ipc->x264enc_agent = agent_create("x264enc", q1, q2);
    ipc->upstream_agent = agent_create("upstream", q2, NULL);
    return ipc;
}

void ipcam_dispatch(struct ipcam *ipc)
{
    agent_dispatch(ipc->videocap_agent, 0);
    agent_dispatch(ipc->display_agent, 0);
    agent_dispatch(ipc->x264enc_agent, 0);
    agent_dispatch(ipc->upstream_agent, 0);
    while (1) {
        sleep(2);
    }
}

static void sigterm_handler(int sig)
{
    agent_destroy(ipcam_instance->videocap_agent);
    agent_destroy(ipcam_instance->display_agent);
    agent_destroy(ipcam_instance->x264enc_agent);
    agent_destroy(ipcam_instance->upstream_agent);
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, sigterm_handler);

    device_register_all();
    display_register_all();
    codec_register_all();
    protocol_register_all();
    agent_register_all();

    ipcam_instance = ipcam_init();
    if (!ipcam_instance) {
        return -1;
    }

    ipcam_dispatch(ipcam_instance);
    return 0;
}
