
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <event.h>
#include <SDL2/SDL.h>

typedef struct stream_channel {
    struct event *ev;
    int fd;

} stream_channel_t;

typedef struct display_filter {
    struct SDL_Window *sdl_window;
    struct SDL_Renderer *sdl_renderer;
    struct SDL_Texture *sdl_texture;

} display_filter_t;

typedef struct player {
    struct event_base *evbase;
    struct display_filter *display;

} player_t;

static player_t *player_instace = NULL;

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

    return;
}

static stream_channel_t *peer_channel_create(player_t *p)
{
    int fd = 0;
    stream_channel_t *sc = NULL;
    struct event *ev = NULL;

    fd = peer_session_create();
    if (fd == -1) {
        return NULL;
    }
    ev = event_new(p->evbase, fd, EV_READ | EV_PERSIST, on_peer_read, sc);
    if (ev == NULL) {
        peer_session_destory(fd);
        return NULL;
    }
    sc= (stream_channel_t *)calloc(1, sizeof(stream_channel_t));
    if (!sc) {
        peer_session_destory(fd);
        event_free(ev);
        return NULL;
    }
    if (event_base_set(p->evbase, ev) ||
        event_add(ev, NULL)) {
        peer_session_destory(fd);
        event_free(ev);
        return NULL;
    }

    sc->ev = ev;
    sc->fd = fd;

    return sc;
}

static void peer_channel_destory(stream_channel_t *sc)
{
    if (sc == NULL)
        return;
    peer_session_destory(sc->fd);
    event_free(sc->ev);

}

static display_filter_t *display_filter_create(int width, int height)
{
    struct SDL_Window *sdl_window = NULL;
    struct SDL_Renderer *sdl_renderer = NULL;
    struct SDL_Texture *sdl_texture = NULL;
    display_filter_t *df = NULL;

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }
    
    sdl_window = SDL_CreateWindow(NULL, 0, 0, width, height,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (sdl_window == NULL) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return NULL;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    if (sdl_renderer == NULL) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return NULL;
    }

    sdl_texture = SDL_CreateTexture(sdl_renderer,
                                    SDL_PIXELFORMAT_YV12,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    width, height);
    if (sdl_texture == NULL) {
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
        return NULL;
    }
    df = (display_filter_t *)calloc(1, sizeof(display_filter_t));
    if (df == NULL) {
        return NULL;
    }
    df->sdl_window = sdl_window;
    df->sdl_renderer = sdl_renderer;
    df->sdl_texture = sdl_texture;
    return df;
}

static void display_filter_destory(display_filter_t *df)
{
    if (df == NULL)
        return;

    SDL_DestroyTexture(df->sdl_texture);
    SDL_DestroyRenderer(df->sdl_renderer);
    SDL_DestroyWindow(df->sdl_window);
    SDL_Quit();
}


static player_t *player_init()
{
    int width = 1024;
    int height = 768;
    player_t *p = NULL;
    struct event_base *evbase = NULL;
    display_filter_t *display = NULL;

    evbase = event_base_new();
    if (!evbase) {
        return NULL;
    }

    p = (player_t *)calloc(1, sizeof(player_t));
    if (!p) {
        event_base_free(evbase);
        return NULL;
    }

    display = display_filter_create(width, height);
    if (!display) {
        return NULL;
    }

    p->evbase = evbase;
    p->display = display;
    peer_channel_create(p);

    return p;
}

static int player_loop(player_t *p)
{
    event_base_loop(p->evbase, 0);
    return 0;
}

static int player_quit()
{

    return 0;
}

int main(int argc, char **argv)
{
    player_instace = player_init();
    player_loop(player_instace);
    player_quit();
    return 0;
}
