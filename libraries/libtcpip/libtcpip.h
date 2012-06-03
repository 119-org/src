#ifndef _LIBTCPIP_H_
#define _LIBTCPIP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef _WIN32_
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")

#elif _LINUX_
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
#define WSACleanup()	do{}while(0);
#define WSAGetLastError()	0
#endif

#define LOCAL_HOST	"127.0.0.1"

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_MSG_MAX_LEN		1460	//MTU
#define TCP_MSG_MAX_LEN		1452

int socket_init();
void socket_close(SOCKET* s);
int socket_shutdown();

int tcp_socket(SOCKET* s);
int tcp_bind(SOCKET* s, u_short port, char* ip);
int tcp_listen(SOCKET* s);
int tcp_accept(SOCKET* s, struct sockaddr_in* preomte_addr, SOCKET* rs);
int tcp_connect(SOCKET* s, char* ip, u_short port);
int tcp_send(SOCKET* s, void* buf);
int tcp_recv(SOCKET* s, void* buf);

int tcp_server_start(u_short server_port);
int tcp_client_start(char* ip, u_short port, char* send_buf);
int udp_sendto(SOCKET* s, void* buf, u_long remote, u_short port);
int udp_recvfrom(SOCKET* s, char* buf, char* ip, u_short port);

int get_addrinfo();

typedef void (*cb_msg_callback) (SOCKET* s, int sendrecv);

int set_cb_msg(cb_msg_callback cbmsg);
#define udp_bind(s, port) tcp_bind(s, port)


#ifdef __cplusplus
}
#endif

#endif
