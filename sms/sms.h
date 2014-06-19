#ifndef _SMS_H_
#define _SMS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct sms {

};

struct buffer {
    void *addr;
    int len;
    int ref_cnt;
};

int sms_init();
int sms_loop();



#ifdef __cplusplus
}
#endif
#endif
