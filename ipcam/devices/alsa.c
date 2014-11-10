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
    int period_size;

} alsa_ctx_t;


static int alsa_open(struct device_ctx *dc, const char *dev)
{
    int res;
    snd_pcm_t *sp;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_t format;
    format = SND_PCM_FORMAT_S16_LE;
    snd_pcm_uframes_t buffer_size, period_size;
    uint32_t sample_rate = 16000;//8000;
    int channels = 1;

    struct alsa_ctx *ac = (struct alsa_ctx *)dc->priv;
    res = snd_pcm_open(&sp, dev, SND_PCM_STREAM_CAPTURE, 0);
    if (res < 0) {
        printf("snd_pcm_open %s failed: %s\n", dev, snd_strerror(res));
        return -1;
    }

    res = snd_pcm_hw_params_malloc(&hw_params);
    if (res < 0) {
        printf("cannot allocate hardware parameter structure: %s\n", snd_strerror(res));
        goto fail1;
    }

    res = snd_pcm_hw_params_any(sp, hw_params);
    if (res < 0) {
        printf("cannot initialize hardware parameter structure: %s\n", snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_access(sp, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0) {
        printf("cannot set access type: %s\n", snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_format(sp, hw_params, format);
    if (res < 0) {
        printf("cannot set sample format %d: %s\n", format, snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_rate_near(sp, hw_params, &sample_rate, 0);
    if (res < 0) {
        printf("cannot set sample rate: %s\n", snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_channels(sp, hw_params, channels);
    if (res < 0) {
        printf("cannot set channel count to %d: %s\n", channels, snd_strerror(res));
        goto fail;
    }

#define ALSA_BUFFER_SIZE_MAX 65536
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

    snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_size);
    buffer_size = FFMIN(buffer_size, ALSA_BUFFER_SIZE_MAX);
    /* TODO: maybe use ctx->max_picture_buffer somehow */
    res = snd_pcm_hw_params_set_buffer_size_near(sp, hw_params, &buffer_size);
    if (res < 0) {
        printf("cannot set ALSA buffer size: %s\n", snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_get_period_size_min(hw_params, &period_size, NULL);
    if (!period_size)
        period_size = buffer_size / 4;
    res = snd_pcm_hw_params_set_period_size_near(sp, hw_params, &period_size, NULL);
    if (res < 0) {
        printf("cannot set ALSA period size (%s)\n", snd_strerror(res));
        goto fail;
    }
    ac->period_size = period_size;

    res = snd_pcm_hw_params(sp, hw_params);
    if (res < 0) {
        printf("cannot set parameters (%s)\n", snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_free(hw_params);

    ac->sp = sp;
    return 0;

fail:
    snd_pcm_hw_params_free(hw_params);
fail1:
    snd_pcm_close(sp);
    return -1;
}

static int alsa_read(struct device_ctx *dc, void *buf, int len)
{
    int res;
    struct alsa_ctx *ac = (struct alsa_ctx *)dc->priv;

    while ((res = snd_pcm_readi(ac->sp, buf, len)) < 0) {
        if (res == -EAGAIN) {
            return -1;
        }
        if (res == -EPIPE) {
            res = snd_pcm_prepare(ac->sp);
            if (res < 0) {
                printf("cannot recover from underrun (snd_pcm_prepare failed: %s)\n", snd_strerror(res));
                return -1;
            }
        } else if (res == -ESTRPIPE) {
            printf("-ESTRPIPE... Unsupported!\n");
            return -1;
        }
    }
    return 0;
}

static int alsa_write(struct device_ctx *dc, void *buf, int len)
{

    return 0;
}

static void alsa_close(struct device_ctx *dc)
{
    struct alsa_ctx *ac = (struct alsa_ctx *)dc->priv;
    snd_pcm_close(ac->sp);
}

struct device ipc_alsa_device = {
    .name = "alsa",
    .open = alsa_open,
    .read = alsa_read,
    .write = alsa_write,
    .close = alsa_close,
    .priv_size = sizeof(struct alsa_ctx),
};
