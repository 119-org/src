#ifndef _LIBTCPIP_H_
#define _LIBTCPIP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")

#elif _LINUX
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>
#include <asm/ioctls.h>
#include <errno.h>
typedef int SOCKET;
#define INVALID_SOCKET	-1
#define SOCKET_ERROR	-1

#endif

#define LOCAL_HOST	"127.0.0.1"
#define ANY_IP		"0.0.0.0"

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_MSG_MAX_LEN		1460	//MTU
#define TCP_MSG_MAX_LEN		1452

int socket_init();
void socket_close(SOCKET* s);
int dbg_socket_error();
int socket_shutdown();

int tcp_socket(SOCKET* s);
int tcp_bind(SOCKET* s, u_short port, char* ip);
int tcp_listen(SOCKET* s);
int tcp_accept(SOCKET* s, struct sockaddr_in* preomte_addr, SOCKET* rs);
int tcp_connect(SOCKET* s, char* ip, u_short port);
int tcp_send(SOCKET* s, char* buf, u_long buf_len);
int tcp_recv(SOCKET* s, char* buf, u_long buf_len);
int tcp_server_start(u_short server_port);
int tcp_client_start(char* ip, u_short port, char* send_buf);


int udp_socket(SOCKET* s);
#define udp_bind	tcp_bind
int udp_sendto(SOCKET* s, void* buf, u_long remote, u_short port);
int udp_recvfrom(SOCKET* s, char* buf, char* ip, u_short port);
int udp_client_start(char* ip, u_short port, char* send_buf);
int udp_server_start(u_short server_port);

int get_addrinfo();

typedef int (*cb_msg_callback)(SOCKET* s, char* buf, char* ip, u_short port);


#ifdef __cplusplus
}
#endif

#endif
