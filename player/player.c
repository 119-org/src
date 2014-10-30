
#include <event.h>
#include <SDL2/SDL.h>

typedef struct player {
    struct event_base *evbase;

} player_t;

typedef struct stream_channel {
    struct event *ev;
    int fd;

} stream_channel_t;

static int peer_session_create()
{
    int fd = 0;
    uint16_t port = 2333;
    char *host = "127.0.0.1";

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return -1;
    }

    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        close(fd);
        return -1;
    }
    return fd;
}

void peer_session_destory(int fd)
{
    close(fd);
}


static void on_peer_read(int fd, short event, void *arg)
{
    stream_channel_t *sc = (stream_channel_t *)arg;

}

static stream_channel_t *peer_channel_create(player_t *p)
{
    int fd = 0;
    stream_channel_t *sc = NULL;
    struct event *e = NULL;

    fd = peer_session_create();
    if (fd == -1) {
        return NULL;
    }
    e = event_new(p->evbase, fd, EV_READ | EV_PERSIST, on_peer_read, sc);
    if (e == NULL) {
        peer_session_destory(fd);
        return NULL;
    }
    sc= (stream_channel_t *)calloc(1, sizeof(stream_channel_t));
    if (!sc) {
        peer_session_destory(fd);
        event_free(e);
        return NULL;
    }
    sc->ev = e;
    sc->fd = fd;

    return sc;
}

static void peer_channel_destory(stream_channel_t *sc)
{

}

static player_t *player_init()
{
    player_t *p = NULL;
    struct event_base *base = NULL;

    base = event_base_new();
    if (!base) {
        return NULL;
    }

    p = (player_t *)calloc(1, sizeof(player_t));
    if (!p) {
        event_base_free(base);
        return NULL;
    }
    p->evbase = base;

    return p;
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
