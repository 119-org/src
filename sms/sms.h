#ifndef _SMS_H_
#define _SMS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HOST	"192.168.1.100"
#define RTSP_PORT 1234

struct sms {
    int socket;

};

struct source {

};

struct sink {

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
