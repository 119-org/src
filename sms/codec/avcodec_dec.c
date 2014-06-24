#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include "codec.h"
#include "common.h"
#include "debug.h"


struct avcodec_ctx {
    int width;
    int height;
    AVCodecContext *avctx;
    AVCodec *avcdc;
    AVFrame *avfrm[2];
    AVFrame *avfrmYUV[2];
    AVPacket *avpkt;

};

int avcdc_open(struct codec_ctx *cc, int width, int height)
{
    struct avcodec_ctx *c = cc->priv;
    int i;
    const uint8_t *out_buffer[2];
    AVFrame *avfrm[2];
    AVFrame *avfrmYUV[2];
    AVPacket *avpkt;
    av_register_all();
    AVCodec *avcdc = avcodec_find_decoder(CODEC_ID_H264);
    if (!avcdc) {
        err("avcodec_find_decoder failed!\n");
        return -1;
    }
    AVCodecContext *avctx = avcodec_alloc_context3(avcdc);
    if (!avctx) {
        err("can not alloc avcodec context!\n");
        return -1;
    }
    /*configure codec context */
    avctx->time_base.num = 1;
    avctx->time_base.den = 25;
    avctx->bit_rate = 0;
    avctx->frame_number = 1;//one frame per package
    avctx->codec_type = AVMEDIA_TYPE_VIDEO;
    avctx->width = width;
    avctx->height = height;

    /*open codec*/
    if (0 > avcodec_open2(avctx, avcdc, NULL)) {
        err("can not open avcodec!\n");
        return -1;
    }

    for (i = 0; i < 2; i++) {
//        avfrm[i] = frame_alloc();
//        avfrmYUV[i] = frame_alloc();
    }
    avpkt = (AVPacket *)av_malloc(sizeof(AVPacket));

/*preparation for frame convertion*/

    for (i = 0; i < 2; i++) {
        out_buffer[i] = (const uint8_t *)malloc(avpicture_get_size(PIX_FMT_YUV420P, avctx->width, avctx->height) * sizeof(uint8_t));
        avpicture_fill((AVPicture *)avfrmYUV[i], out_buffer[i], PIX_FMT_YUV420P, avctx->width, avctx->height);
    }
}

int avcdc_decode(struct codec_ctx *cc, int width, int height)
{

}

struct codec cdc_avcdc_decoder = {
    .name = "avcdc",
    .open = avcdc_open,
    .encode = NULL,
    .decode = avcdc_decode,
    .priv_size = sizeof(struct avcodec_ctx),
};
