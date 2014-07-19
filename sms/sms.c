#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include "debug.h"
//#include "source.h"
//#include "sink.h"
//#include "codec.h"
#include "common.h"
#include "sms.h"

#define MAX_INPUT_LEN	128
#define MAX_OUTPUT_LEN	128

static char input[MAX_INPUT_LEN];
static char output[MAX_OUTPUT_LEN];
static struct sms *g_sms = NULL;

int sms_init(struct sms *sms)
{
    struct source_ctx *src = sms->src;
    struct sink_ctx *snk = sms->snk;
    struct codec_ctx *enc = sms->enc;
    struct codec_ctx *dec = sms->dec;
    if (-1 == source_open(src)) {
        err("source_open failed!\n");
        return -1;
    }
    if (-1 == codec_open(enc, src->width, src->height)) {
        err("codec_open failed!\n");
        return -1;
    }
    if (-1 == codec_open(dec, src->width, src->height)) {
        err("codec_open failed!\n");
        return -1;
    }
    if (-1 == sink_open(snk)) {
        err("sink_open failed!\n");
        return -1;
    }
    return 0;
}

int sms_loop(struct sms *sms)
{
    struct source_ctx *src = sms->src;
    struct sink_ctx *snk = sms->snk;
    struct codec_ctx *enc = sms->enc;
    struct codec_ctx *dec = sms->dec;
    int ret, len;
    int flen = 0x100000;
    void *frm = calloc(1, flen);
    void *pkt = calloc(1, flen);
    void *yuv = frm;

    while (1) {
        if (-1 == sink_poll(snk)) {
            err("sink poll failed!\n");
            continue;
        }
        sink_handle(snk);
        len = source_read(src, frm, flen);
        if (len == -1) {
            err("source read failed!\n");
            continue;
        }
        len = codec_encode(enc, frm, pkt);
        if (len == -1) {
            err("encode failed!\n");
            continue;
        }
        ret = codec_decode(dec, pkt, len, &yuv);
        if (ret == -1) {
            err("decode failed!\n");
            continue;
        }
        if (-1 == sink_write(snk, yuv, len)) {
            err("sink write failed!\n");
            continue;
        }
        if (-1 == source_write(src, NULL, 0)) {
            err("source write failed!\n");
            continue;
        }
    }

    return 0;
}

static struct option long_options[] =
{
    {"input", required_argument, 0, 'i'},
    {"output", required_argument, 0, 'o'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf("Usage:\n"
           "Required option:\n"
           "-i <input>\n"
           "--input <source>\n"
           "-o <output>\n"
           "--output <sink>\n"
           "$ ./sms -i v4l:///dev/video0 -o sdl://player\n"
           );
    exit(-1);
}
int main(int argc, char **argv)
{
    int c;
    int opt_index = 0;
    while (1) {
        c = getopt_long(argc, argv, "i:o:", long_options, &opt_index);
        if (c == -1) {
            if (optind == 1) {
                usage();
            }
            break;
        }
        switch (c) {
        case 0:
            printf ("option %s", long_options[opt_index].name);
            if (long_options[opt_index].flag != 0) {
                break;
            }
            if (optarg) {
                printf (" with arg %s", optarg);
            }
            printf ("\n");
            break;
        case 'i':
            strcpy(input, optarg);
            break;
        case 'o':
            strcpy(output, optarg);
            break;
        case '?':
            break;
        default:
            usage();
            break;
        }
    }
    dbg("output: %s\n", output);
    dbg("input: %s\n", input);
    source_register_all();
    sink_register_all();
    codec_register_all();

    g_sms = (struct sms *)calloc(1, sizeof(struct sms));
    g_sms->src = source_init(input);
    g_sms->snk = sink_init(output);
    g_sms->enc = codec_init("x264");
    g_sms->dec = codec_init("avcodec");

    sms_init(g_sms);
    sms_loop(g_sms);

    return 0;
}
