#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <alsa/asoundlib.h>

#include "device.h"
#include "common.h"
#include "debug.h"



typedef struct alsa_ctx {
    snd_pcm_t *sp;

} alsa_ctx_t;


static int alsa_open(struct device_ctx *dc, const char *dev)
{
    int res;
    snd_pcm_t *sp;
    snd_pcm_hw_params_t *hw_params;
    struct alsa_ctx *ac = (struct alsa_ctx *)dc->priv;
    res = snd_pcm_open(&sp, dev, SND_PCM_STREAM_CAPTURE, 0);
    if (res < 0) {
        printf("snd_pcm_open %s failed: %s\n", dev, strerror(errno));
        return -1;
    }

    res = snd_pcm_hw_params_malloc(&hw_params);
    if (res < 0) {
        printf("cannot allocate hardware parameter structure: %s\n", strerror(errno));
        goto fail1;
    }

    res = snd_pcm_hw_params_any(sp, hw_params);
    if (res < 0) {
        printf("cannot initialize hardware parameter structure: %s\n", strerror(errno));
        goto fail;
    }

    res = snd_pcm_hw_params_set_access(sp, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0) {
        printf("cannot set access type: %s\n", strerror(errno));
        goto fail;
    }

    format = SND_PCM_FORMAT_S16_LE;
    res = snd_pcm_hw_params_set_format(sp, hw_params, format);
    if (res < 0) {
        printf("cannot set sample format %d: %s\n", format, strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_rate_near(h, hw_params, sample_rate, 0);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set sample rate (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_channels(h, hw_params, channels);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set channel count to %d (%s)\n",
               channels, snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_size);
    buffer_size = FFMIN(buffer_size, ALSA_BUFFER_SIZE_MAX);
    /* TODO: maybe use ctx->max_picture_buffer somehow */
    res = snd_pcm_hw_params_set_buffer_size_near(h, hw_params, &buffer_size);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set ALSA buffer size (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_get_period_size_min(hw_params, &period_size, NULL);
    if (!period_size)
        period_size = buffer_size / 4;
    res = snd_pcm_hw_params_set_period_size_near(h, hw_params, &period_size, NULL);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set ALSA period size (%s)\n",
               snd_strerror(res));
        goto fail;
    }
    s->period_size = period_size;

    res = snd_pcm_hw_params(h, hw_params);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set parameters (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_free(hw_params);

    if (channels > 2 && layout) {
        if (find_reorder_func(s, *codec_id, layout, mode == SND_PCM_STREAM_PLAYBACK) < 0) {
            char name[128];
            av_get_channel_layout_string(name, sizeof(name), channels, layout);
            av_log(ctx, AV_LOG_WARNING, "ALSA channel layout unknown or unimplemented for %s %s.\n",
                   name, mode == SND_PCM_STREAM_PLAYBACK ? "playback" : "capture");
        }
        if (s->reorder_func) {
            s->reorder_buf_size = buffer_size;
            s->reorder_buf = av_malloc(s->reorder_buf_size * s->frame_size);
            if (!s->reorder_buf)
                goto fail1;
        }
    }

    s->h = h;
    return 0;

fail:
    snd_pcm_hw_params_free(hw_params);
fail1:
    snd_pcm_close(h);
    return AVERROR(EIO);
    return 0;
}

static int alsa_read(struct device_ctx *dc, void *buf, int len)
{

    return 0;
}

static int alsa_write(struct device_ctx *dc, void *buf, int len)
{

    return 0;
}

static void alsa_close(struct device_ctx *dc)
{

}

struct device ipc_alsa_device = {
    .name = "alsa",
    .open = alsa_open,
    .read = alsa_read,
    .write = alsa_write,
    .close = alsa_close,
    .priv_size = sizeof(struct alsa_ctx),
};
