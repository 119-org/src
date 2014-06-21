#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "sink.h"
#include "common.h"
#include "debug.h"

const char *wnd_title = "Stream Media Player v0.1";

enum wnd_type {
    SDL_RGB,
    SDL_YUV
};

struct sdl_rgb_ctx {
    SDL_Surface *surface;
    uint32_t rmask;
    uint32_t gmask;
    uint32_t bmask;
    uint32_t amask;
    int width;
    int height;
    int bpp;
    int pitch;
    uint8_t *pixels;
    int pixels_num;
};

struct sdl_yuv_ctx {

};

struct sdl_ctx {
    int index;
    uint8_t *src_buf;
    int width;
    int height;
    int bpp;
    struct sdl_rgb_ctx *rgb;
    SDL_Surface *surface;
    SDL_Event event;
};

static void sdl_deinit()
{
    SDL_Quit();
    exit(0);
}

static int clamp(double x)
{
    int r = x;
    if (r < 0)
        return 0;
    else if (r > 255)
        return 255;
    else
        return r;
}

static void yuv2rgb(uint8_t Y, uint8_t Cb, uint8_t Cr, int *ER, int *EG, int *EB)
{
    double y1, pb, pr, r, g, b;
    y1 = (255 / 219.0) * (Y - 16);
    pb = (255 / 224.0) * (Cb - 128);
    pr = (255 / 224.0) * (Cr - 128);
    r = 1.0 * y1 + 0 * pb + 1.402 * pr;
    g = 1.0 * y1 - 0.344 * pb - 0.714 * pr;
    b = 1.0 * y1 + 1.722 * pb + 0 * pr;
    *ER = clamp(r);
    *EG = clamp(g);
    *EB = clamp(b);
}

static int rgb_surface_init(struct sdl_ctx *c)
{
    struct sdl_rgb_ctx *rgb = (struct sdl_rgb_ctx *)calloc(1, sizeof(struct sdl_rgb_ctx));
    rgb->rmask = 0x000000ff;
    rgb->gmask = 0x0000ff00;
    rgb->bmask = 0x00ff0000;
    rgb->amask = 0xff000000;
    rgb->width = c->width;
    rgb->height = c->height;
    rgb->bpp = c->bpp;
    rgb->pitch = c->width * 4;
    rgb->pixels_num = c->width * c->height * 4;
    rgb->pixels = (uint8_t *)calloc(1, rgb->pixels_num);
    rgb->surface = SDL_CreateRGBSurfaceFrom(rgb->pixels,
                          rgb->width,
                          rgb->height,
                          rgb->bpp,
                          rgb->pitch,
                          rgb->rmask,
                          rgb->gmask,
                          rgb->bmask,
                          rgb->amask);
    c->rgb = rgb;
    return 0;
}

static void rgb_pixels_update(struct sdl_ctx *c)
{
    uint8_t *data = c->src_buf;
    uint8_t *pixels = c->rgb->pixels;
    int width = c->rgb->width;
    int height = c->rgb->height;
    uint8_t Y, Cr, Cb;
    int r, g, b;
    int x, y;
    int p1, p2, p3, p4;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            p1 = y * width * 2 + x * 2;
            Y = data[p1];
            if (x % 2 == 0) {
                p2 = y * width * 2 + (x * 2 + 1);
                p3 = y * width * 2 + (x * 2 + 3);
            } else {
                p2 = y * width * 2 + (x * 2 - 1);
                p3 = y * width * 2 + (x * 2 + 1);
            }
            Cb = data[p2];
            Cr = data[p3];
            yuv2rgb(Y, Cb, Cr, &r, &g, &b);
            p4 = y * width * 4 + x * 4;
            pixels[p4] = r;
            pixels[p4 + 1] = g;
            pixels[p4 + 2] = b;
            pixels[p4 + 3] = 255;
        }
    }
}
static int rgb_surface_update(struct sdl_ctx *c)
{
    rgb_pixels_update(c);
    SDL_BlitSurface(c->rgb->surface, NULL, c->surface, NULL);
    SDL_Flip(c->surface);
    return 0;
}

static int wnd_init(struct sdl_ctx *c, int type)
{
    int flags = SDL_SWSURFACE | SDL_DOUBLEBUF;
    flags |= SDL_RESIZABLE;
    c->width = 640;
    c->height = 480;
    c->bpp = 32;
    c->surface = SDL_SetVideoMode(c->width, c->height, c->bpp, flags);
    if (c->surface == NULL) {
        err("SDL: could not set video mode - exiting\n");
        return -1;
    }
    rgb_surface_init(c);
    return 0;
}

static int sdl_init(struct sdl_ctx *c)
{
    int flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTTHREAD;
    if (-1 == SDL_Init(flags)) {
        err("Could not initialize SDL - %s\n", SDL_GetError());
        goto fail;
    }
    SDL_WM_SetCaption(wnd_title, NULL);
    if (-1 == wnd_init(c, SDL_RGB)) {
        err("Could not initialize RGB surface\n");
        goto fail;
    }
    return 0;

fail:
    sdl_deinit();
    return -1;
}

static int sdl_open(struct sink_ctx *sc)
{
    struct sdl_ctx *c = sc->priv;
    sdl_init(c);
    return 0;
}

static int sdl_read(struct sink_ctx *sc, void *buf, int len)
{

    return 0;
}

static int sdl_write(struct sink_ctx *sc, void *buf, int len)
{
    struct sdl_ctx *c = sc->priv;
    struct frame *f = (struct frame *)buf;
    c->src_buf = f->addr;
    rgb_surface_update(c);

    return 0;
}

static void sdl_close(struct sink_ctx *sc)
{
    sdl_deinit();
}

struct sink snk_sdl_module = {
    .name = "sdl",
    .open = sdl_open,
    .read = sdl_read,
    .write = sdl_write,
    .close = sdl_close,
    .priv_size = sizeof(struct sdl_ctx),
};
