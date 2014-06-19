#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "sink.h"

struct sdl_ctx {

};

static int sdl_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTTHREAD) < 0) {
        perror("SDL_Init");
        return -1;
    }
    SDL_WM_SetCaption("Simple WebCam", NULL);
    atexit(SDL_Quit);
    return 0;
}

static int sdl_open(struct sink_ctx *c)
{
    sdl_init();
    return 0;
}

static int sdl_read(struct sink_ctx *c, uint8_t *buf, int len)
{

    return 0;
}

static int sdl_write(struct sink_ctx *c, uint8_t *buf, int len)
{

    return 0;
}

static void sdl_close(struct sink_ctx *c)
{

    return 0;
}

struct sink sdl_sink = {
    .name = "v4l",
    .sink_open = sdl_open,
    .sink_read = sdl_read,
    .sink_write = sdl_write,
    .sink_close = sdl_close,
    .priv_size = sizeof(struct sdl_ctx),
};
