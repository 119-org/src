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
    AVFrame *avfrm;
    AVFrame *avfrmYUV;
    AVPacket *avpkt;
};

int avcodec_init(struct codec_ctx *cc, int width, int height)
{
    struct avcodec_ctx *c = cc->priv;
    int i;
    const uint8_t *out_buffer;
    AVFrame *avfrm;
    AVFrame *avfrmYUV;
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

//    avfrm = av_frame_alloc();
//    avfrmYUV = av_frame_alloc();
    avpkt = (AVPacket *)av_malloc(sizeof(AVPacket));

/*preparation for frame convertion*/

    out_buffer = (const uint8_t *)malloc(avpicture_get_size(PIX_FMT_YUV420P, avctx->width, avctx->height) * sizeof(uint8_t));
    avpicture_fill((AVPicture *)avfrmYUV, out_buffer, PIX_FMT_YUV420P, avctx->width, avctx->height);
    c->width = width;
    c->height = height;
    c->avctx = avctx;
    c->avcdc = avcdc;
    c->avpkt = avpkt;
    c->avfrm = avfrm;
    c->avfrmYUV = avfrmYUV;
    return 0;
}

static int frame_conv(struct avcodec_ctx *c)
{
    struct SwsContext *swsctx;
    swsctx = sws_getContext(c->avctx->width, c->avctx->height, c->avctx->pix_fmt, c->avctx->width, c->avctx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(swsctx, (const uint8_t* const*)c->avfrm->data, c->avfrm->linesize, 0, c->avctx->height, c->avfrmYUV->data, c->avfrmYUV->linesize);
    sws_freeContext(swsctx);
    return 0;
}

int avcodec_decode(struct codec_ctx *cc, void *in, int *inlen, void *out, int *outlen)
{
    struct avcodec_ctx *c = cc->priv;
    int got_pic = 0;
    c->avpkt->size = inlen;
    c->avpkt->data = (uint8_t *)in;

    avcodec_decode_video2(c->avctx, c->avfrm, (int *)&got_pic, c->avpkt);
    if (got_pic) {
        frame_conv(c);
    }
    return got_pic;
}

struct codec cdc_avcodec_decoder = {
    .name = "avcodec",
    .open = avcodec_init,
    .encode = NULL,
    .decode = avcodec_decode,
    .priv_size = sizeof(struct avcodec_ctx),
};
