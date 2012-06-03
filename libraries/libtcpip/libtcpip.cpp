#include "libtcpip.h"
#define ENABLE	1
#define DISABLE	0

int socket_init()
{
#ifdef _WIN32_
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(ret != NO_ERROR)
	{
		fprintf(stdout, "Error at WSAStartup(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
#elif _LINUX_

#endif
	return 0;
}
void socket_close(SOCKET* s)
{
#ifdef _WIN32_
	closesocket(*s);
#elif _LINUX_
	close(*s);
#endif

}
int tcp_socket(SOCKET* s)
{
	u_long noblock = DISABLE;
	*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(*s == INVALID_SOCKET)
	{
		fprintf(stdout, "Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
#ifdef _WIN32_
	if(ioctlsocket(*s, FIONBIO, &noblock) == SOCKET_ERROR)
#elif _LINUX_
	if(ioctl(*s, FIONBIO, &noblock) == SOCKET_ERROR)
#endif
	{
		socket_close(s);
		fprintf(stdout, "Error at ioctlsocket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	return 0;
}

int tcp_bind(SOCKET* s, u_short port, char* ip = LOCAL_HOST)
{
	struct sockaddr_in addr;
	struct hostent* host = (struct hostent*)gethostbyname(ip);
	char* localhost = inet_ntoa(*(struct in_addr*)*host->h_addr_list);
	if(*s == NULL || port < 0 || port > 65535)
	{
		fprintf(stdout, "Paraments error at tcp_bind()\n");
		return -1;
	}


	addr.sin_family = AF_INET;
//	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_addr.s_addr = inet_addr(localhost);
	addr.sin_port = htons(port);
//	bzero(&(addr.sin_zero), 8);
	if(bind(*s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		socket_close(s);
		fprintf(stdout, "Error at bind(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	return 0;
}

int tcp_listen(SOCKET* s)
{
	int iOptVal;
	int iOptLen = sizeof(int);

	if(getsockopt(*s, SOL_SOCKET, SO_KEEPALIVE, (char*)&iOptVal,(socklen_t*)&iOptLen) != SOCKET_ERROR)
	{
		fprintf(stdout, "SO_KEEPALIVE Value: %ld\n", iOptVal);
	}
	if(listen(*s, SOMAXCONN) == SOCKET_ERROR)
	{
		socket_close(s);
		fprintf(stdout, "Error at listen(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	if(getsockopt(*s, SOL_SOCKET, SO_KEEPALIVE, (char*)&iOptVal, (socklen_t*)&iOptLen) != SOCKET_ERROR)
	{
		fprintf(stdout, "SO_KEEPALIVE Value: %ld\n", iOptVal);
	}
	return 0;
}

int tcp_accept(SOCKET* s, struct sockaddr_in* premote, SOCKET* as)
{
	int addrlen = sizeof(struct sockaddr_in);
	char* remote = inet_ntoa(premote->sin_addr);
	memset(premote, 0, sizeof(struct sockaddr_in));

	*as = accept(*s, (struct sockaddr*)premote, (socklen_t*)&addrlen);
	if(*as == INVALID_SOCKET)
	{
		socket_close(s);
		fprintf(stdout, "accept failed: %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}


	return 0;
}

int tcp_connect(SOCKET* s, char* ip, u_short port)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	connect(*s, (struct sockaddr*)&addr, sizeof(addr));

	return 0;
}

int tcp_send(SOCKET* s, void* buf)
{
	int ret = send(*s, (char*)buf, strlen((char*)buf), 0);
	if(ret == SOCKET_ERROR)
	{
		socket_close(s);
		fprintf(stdout, "send failed: %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	return ret;
}

int tcp_recv(SOCKET* s, void* buf)
{
	int ret = recv(*s, (char*)buf, strlen((char*)buf), 0);
	if(ret == SOCKET_ERROR)
	{
		fprintf(stdout, "recv failed: %d\n", WSAGetLastError());
		socket_close(s);
		WSACleanup();
		return -1;
	}
	return ret;
}

int udp_socket(SOCKET* s)
{
	*s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(*s == INVALID_SOCKET)
	{
		fprintf(stdout, "Error at socket(): %ld\n", WSAGetLastError());
		socket_close(s);
		WSACleanup();
		return -1;
	}
	return 0;
}

int udp_sendto(SOCKET* s, void* buf, u_long remote, u_short port)
{
	int ret = 0;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = remote;
	addr.sin_port = htons(port);
	ret = sendto(*s, (char*)buf, strlen((char*)buf), 0, (struct sockaddr*)&addr, sizeof(addr));
	if(ret == SOCKET_ERROR)
	{
		fprintf(stdout, "sendto failed: %d\n", WSAGetLastError());
		socket_close(s);
		WSACleanup();
		return -1;
	}
	return 0;
}

int udp_recvfrom(SOCKET* s, char* buf, char* ip, u_short port)
{
	int ret = 0;
	int addrsize = 0;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	addrsize = sizeof(addr);

	ret = recvfrom(*s, (char*)buf, UDP_MSG_MAX_LEN, 0, (struct sockaddr*)&addr, (socklen_t*)&addrsize);
	if(ret == SOCKET_ERROR)
	{
		fprintf(stdout, "recvfrom failed: %d\n", WSAGetLastError());
		if(WSAGetLastError() != 10040)
		{
			socket_close(s);
			WSACleanup();
			return -1;
		}
	}
	return 0;
}

int get_addrinfo(char* ip, char* port)
{
	int ret = 0;
	char ipbuf[16];

	struct addrinfo hints;
	struct addrinfo* list;
	struct addrinfo* cur;
	struct sockaddr_in* addr;

	memset(&hints, 0, sizeof(struct addrinfo));
	memset(&list, 0, sizeof(list));
	hints.ai_family = AF_INET; /* Allow IPv4 */
	hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
	hints.ai_protocol = 0; /* Any protocol */
	hints.ai_socktype = SOCK_STREAM;

	ip = "www.baidu.com";
	port = NULL;

	getaddrinfo(ip, port, &hints, &list);

	for (cur = list; cur != NULL; cur = cur->ai_next) {
		addr = (struct sockaddr_in *)cur->ai_addr;
		printf("%s\n", inet_ntop(AF_INET, 
			&addr->sin_addr, ipbuf, 16));
	}
	return 0;
}

int set_cb_msg(cb_msg_callback cbmsg)
{
	int ret = 0;

//	ret = udp_recvfrom(SOCKET* s, char* buf, char* ip, u_short port, int sendrecv);
	printf("hello \n");
}

int tcp_server_start(u_short server_port)
{
	int i = 0;
	char* recv_buf;
	recv_buf = (char*)malloc(1024);

	SOCKET sock, accept_sock;
	struct sockaddr_in remote_ip;

	socket_init();
	tcp_socket(&sock);
	tcp_bind(&sock, server_port);
	tcp_listen(&sock);
	while(1)
	{
		tcp_accept(&sock, &remote_ip, &accept_sock); 
		tcp_recv(&accept_sock, (void*)recv_buf); 
		printf("tcp client connect times = %d\n", i);
		printf("tcp receive buffer: %s\n", (char*)recv_buf);
		i++;
	}
}
int tcp_client_start(char* ip, u_short port, char* send_buf = NULL)
{
	if(send_buf == NULL)
	{
		send_buf = (char*)malloc(1024);
		memset(send_buf, 0x01020304, 1024);
	}
	SOCKET sock;
	socket_init();
	tcp_socket(&sock);
	tcp_connect(&sock, ip, port);
	tcp_send(&sock, (void*)&send_buf);

}
int main()
{
//	tcp_server_start(5060);
	tcp_client_start("127.0.0.1", 5060);
	return 0;
}
