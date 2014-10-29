
#include <event.h>
#include <SDL2/SDL.h>

typedef struct player {
    struct event_base *evbase;

} player_t;

typedef struct stream_channel {
    struct event *ev;

} stream_channel_t;


static int peer_channel_init()
{

    return 0;
}

static int player_init()
{
    player_t *p = (player_t *)calloc(1, sizeof(player_t));
    if (!p) {
        return -1;
    }
    p->evbase = event_init();

    return 0;
}

static int player_loop()
{

    return 0;
}

static int player_quit()
{

    return 0;
}

int main(int argc, char **argv)
{
    player_init();
    player_loop();
    player_quit();
    return 0;
}
