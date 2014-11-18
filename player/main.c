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
#include "protocol.h"
#include "buffer.h"
#include "codec.h"
#include "network_agent.h"
#include "avcodec_decode_agent.h"
#include "display_agent.h"

typedef struct player {
    struct avcodec_decode_agent *avda;
    struct network_agent *na;
    struct display_agent *da;
} player_t;

static player_t *player_instace = NULL;

void player_dispatch(struct player *p)
{
    network_agent_dispatch(p->na);
    avcodec_decode_agent_dispatch(p->avda);
    display_agent_dispatch(p->da);
    while (1) {
        sleep(2);
    }
}



static int player_quit()
{

    return 0;
}

struct player *player_init()
{
    player_t *player = NULL;

    protocol_register_all();
    codec_register_all();

    player = (player_t *)calloc(1, sizeof(player_t));
    if (!player)
        return NULL;

    struct buffer_ctx *net_buf_src = NULL;
    struct buffer_ctx *net_buf_snk = buffer_create(3);
    player->na = network_agent_create(net_buf_src, net_buf_snk);
    if (!player->na) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
    struct buffer_ctx *dec_buf_src = net_buf_snk;
    struct buffer_ctx *dec_buf_snk = buffer_create(3);
    player->avda = avcodec_decode_agent_create(dec_buf_src, dec_buf_snk);
    if (!player->avda) {
        printf("network_agent_create failed!\n");
        return NULL;
    }

    struct buffer_ctx *dsp_buf_src = dec_buf_snk;
    struct buffer_ctx *dsp_buf_snk = NULL;
    player->da = display_agent_create(dsp_buf_src, dsp_buf_snk);
    if (!player->da) {
        printf("network_agent_create failed!\n");
        return NULL;
    }
    return player;
}

int main(int argc, char **argv)
{
    player_instace = player_init();
    player_dispatch(player_instace);
    player_quit();
    return 0;
}
