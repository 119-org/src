#ifndef __LIBSTUN_H__
#define __LIBSTUN_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t port;
    uint32_t addr;
} StunAddress4;

int stun_init(const char *ip);
int stun_socket(const char *ip, uint16_t port, StunAddress4 *map);
int stun_nat_type();
void stun_keep_alive(int fd);


#ifdef __cplusplus
}
#endif
#endif
