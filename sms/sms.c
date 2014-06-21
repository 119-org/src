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

int sms_init(struct source_ctx *src, struct sink_ctx *snk)
{
    source_open(src);
    sink_open(snk);
    struct frame f;
    int len;

    while (1) {
        len = source_read(src, &f, sizeof(f));
        if (len == -1) {
            continue;
        }
        sink_write(snk, &f, sizeof(f));
        source_write(src, &f, sizeof(f));
    }

    return 0;
}

int sms_loop()
{
    while (1) {
        sleep(1);
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
           "$ ./sms -i v4l://dev/video0 -o sdl://player\n"
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
    printf("output: %s\n", output);
    printf("input: %s\n", input);
    source_register_all();
    sink_register_all();

    struct source_ctx *src = source_init(input);
    struct sink_ctx *snk = sink_init(output);

    sms_init(src, snk);
    sms_loop();

    return 0;
}
