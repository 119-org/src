#ifndef _SMS_H_
#define _SMS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct sms {
    struct source_ctx *src;
    struct sink_ctx *snk;
    struct codec_ctx *cdc;
};

struct buffer {
    void *addr;
    int len;
    int ref_cnt;
};

int sms_init(struct sms *sms);
int sms_loop();



#ifdef __cplusplus
}
#endif
#endif
