#ifndef _LIBPTCP_H_
#define _LIBPTCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct _ptcp_socket ptcp_socket_t;
struct my_struct {
    ptcp_socket_t *ps;
    int fd;
    int epfd;
};
#define MAX_EPOLL_EVENT 16


typedef enum {
    PSEUDO_TCP_DEBUG_NONE = 0,
    PSEUDO_TCP_DEBUG_NORMAL,
    PSEUDO_TCP_DEBUG_VERBOSE,
} ptcp_debug_level_t;


typedef enum {
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_CLOSED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSING,
    TCP_TIME_WAIT,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
} ptcp_state_t;

typedef enum {
    WR_SUCCESS,
    WR_TOO_LARGE,
    WR_FAIL
} ptcp_write_result_t;

typedef enum {
    PSEUDO_TCP_SHUTDOWN_RD,
    PSEUDO_TCP_SHUTDOWN_WR,
    PSEUDO_TCP_SHUTDOWN_RDWR,
} ptcp_shutdown_t;

typedef struct ptcp_callbacks {    
    void *data;
    void (*on_opened)(ptcp_socket_t *p, void *data);
    void (*on_readable)(ptcp_socket_t *p, void *data);
    void (*on_writable)(ptcp_socket_t *p, void *data);
    void (*on_closed)(ptcp_socket_t *p, uint32_t error, void *data);
    ptcp_write_result_t (*write)(ptcp_socket_t *p, const char *buf, uint32_t len, void *data);
} ptcp_callbacks_t;


typedef enum {
    OPT_NODELAY,    // Whether to enable Nagle's algorithm (0 == off)
    OPT_ACKDELAY,   // The Delayed ACK timeout (0 == off).
    OPT_RCVBUF,     // Set the receive buffer size, in bytes.
    OPT_SNDBUF,     // Set the send buffer size, in bytes.
} ptcp_option_t;



ptcp_socket_t *ptcp_create(ptcp_callbacks_t *cbs);
void ptcp_destroy(ptcp_socket_t *p);

int ptcp_connect(ptcp_socket_t *p);
int ptcp_recv(ptcp_socket_t *p, void *buf, size_t len);
int ptcp_send(ptcp_socket_t *p, const void *buf, size_t len);
void ptcp_close(ptcp_socket_t *p, int force);
void ptcp_shutdown(ptcp_socket_t *p, ptcp_shutdown_t how);
int ptcp_get_error(ptcp_socket_t *p);
int ptcp_get_next_clock(ptcp_socket_t *p, uint64_t *timeout);
void ptcp_notify_clock(ptcp_socket_t *p);
void ptcp_notify_mtu(ptcp_socket_t *p, uint16_t mtu);
int ptcp_notify_packet(ptcp_socket_t *p, const char *buf, uint32_t len);
int ptcp_notify_message(ptcp_socket_t *p, void *msg);
void ptcp_set_debug_level(ptcp_debug_level_t level);
int ptcp_get_available_bytes(ptcp_socket_t *p);
int ptcp_can_send(ptcp_socket_t *p);
size_t ptcp_get_available_send_space(ptcp_socket_t *p);
void ptcp_set_time(ptcp_socket_t *p, uint32_t current_time);
int ptcp_is_closed(ptcp_socket_t *p);
int ptcp_is_closed_remotely(ptcp_socket_t *p);
void ptcp_get_option(ptcp_option_t opt, int *value);
void ptcp_set_option(ptcp_option_t opt, int value);

ptcp_write_result_t ptcp_write(ptcp_socket_t *p, const char *buf, uint32_t len, void *data);

ptcp_write_result_t ptcp_read(ptcp_socket_t *p, const char *buf, uint32_t len, void *data);

#ifdef __cplusplus
}
#endif
#endif
