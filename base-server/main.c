
#include <stdio.h>
#include <stdlib.h>
#include "libtcpip.h"

struct _srv_param_t {
	SOCKET sock_s;
	int listen_port;	
	int max_client;
	int timeout;
}srv_param_t;

#if 0
int srv_init()
{
	int ret;
	SOCKET sock;
	ret = socket_init();
	ret = tcp_socket(&sock);
	ret = tcp_bind(&sock, listen_port);
	ret = tcp_listen(&sock, 100);
}
int srv_listen()
{
	
}
#endif

int main(int argc, char* argv[])
{
	int listen_port = 5060;
	int ret;
	SOCKET sock;
	SOCKET sock_accept;
	struct sockaddr_in client;
	struct srv_param_t* srv_conf;
	ret = socket_init();
	ret = tcp_socket(&sock);
	ret = tcp_bind(&sock, listen_port);
	ret = tcp_listen(&sock);

	printf("base-server: hello world\n");
	for (;;) {
		struct timeval tv;		/*超时时间*/
		fd_set rfds;			/*读文件集*/

		/*清读文件集,将客户端连接
		  描述符放入读文件集*/
		FD_ZERO(&rfds);	
		FD_SET(sock, &rfds);	

		/*设置超时*/
		tv.tv_sec = 0;
		tv.tv_usec = 500000;

		ret = select(sock + 1, &rfds, NULL, NULL, &tv);	//maxfdp, readfds, writefds, errorfds, timeout
		switch (ret) {
			case -1:
			case 0:
			default:
				if (FD_ISSET(sock, &rfds)) {
					tcp_accept(&sock, &client, &sock_accept);
				}
				break;
		}
	}
/*
	srv_init();
	srv_listen();
	srv_run();
*/

	return 0;	
}
