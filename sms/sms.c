#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include "debug.h"
#include "source.h"
#include "sink.h"
#include "common.h"
#include "sms.h"

#define MAX_INPUT_LEN	128
#define MAX_OUTPUT_LEN	128

static char input[MAX_INPUT_LEN];
static char output[MAX_OUTPUT_LEN];

int sms_init(struct sms *sms)
{
    struct source_ctx *src = sms->src;
    struct sink_ctx *snk = sms->snk;
    struct codec_ctx *cdc = sms->cdc;
    source_open(src);
    codec_open(cdc, src->width, src->height);
    sink_open(snk);
    return 0;
}

int sms_loop(struct sms *sms)
{
    struct source_ctx *src = sms->src;
    struct sink_ctx *snk = sms->snk;
    struct codec_ctx *cdc = sms->cdc;
    int len;
    int flen = 0x100000;
    void *frm = calloc(1, flen);
    void *pkt = calloc(1, flen);

    while (1) {
        sink_poll(snk);
        sink_handle(snk);
        len = source_read(src, frm, flen);
        if (len == -1) {
            err("source read failed!\n");
        }
        len = codec_encode(cdc, frm, pkt);
        if (len == -1) {
            err("encode failed!\n");
        }
//        dbg("codec_encode len = %d\n", len);
        sink_write(snk, pkt, len);
        source_write(src, NULL, 0);
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

    struct source_ctx *src = source_init(input);
    struct sink_ctx *snk = sink_init(output);
    struct codec_ctx *cdc = codec_init("x264");
    struct sms *sms = (struct sms *)calloc(1, sizeof(struct sms));
    sms->src = src;
    sms->snk = snk;
    sms->cdc = cdc;

    sms_init(sms);
    sms_loop(sms);

    return 0;
}
