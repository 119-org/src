#include "libtcpip.h"
#define ENABLE	1
#define DISABLE	0
int init_socket()
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(ret != NO_ERROR)
	{
		fprintf(stdout, "Error at WSAStartup(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	return 0;
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

	if(ioctlsocket(*s, FIONBIO, &noblock) == SOCKET_ERROR)
	{
		closesocket(*s);
		fprintf(stdout, "Error at ioctlsocket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	return 0;
}

int tcp_bind(SOCKET* s, u_short port)
{
	struct sockaddr_in addr;
	struct hostent* host = (struct hostent*)gethostbyname("");
	char* localhost = inet_ntoa(*(struct in_addr*)*host->h_addr_list);
	if(*s == NULL || port < 0 || port > 65535)
	{
		fprintf(stdout, "Paraments error at tcp_bind()\n");
		return -1;
	}


	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(localhost);
	addr.sin_port = htons(port);

	if(bind(*s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		closesocket(*s);
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

	if(getsockopt(*s, SOL_SOCKET, SO_KEEPALIVE, (char*)&iOptVal, &iOptLen) != SOCKET_ERROR)
	{
		fprintf(stdout, "SO_KEEPALIVE Value: %ld\n", iOptVal);
	}
	if(listen(*s, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(*s);
		fprintf(stdout, "Error at listen(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	if(getsockopt(*s, SOL_SOCKET, SO_KEEPALIVE, (char*)&iOptVal, &iOptLen) != SOCKET_ERROR)
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

	*as = accept(*s, (struct sockaddr*)premote, &addrlen);
	if(*as == INVALID_SOCKET)
	{
		fprintf(stdout, "accept failed: %d\n", WSAGetLastError());
		closesocket(*s);
		WSACleanup();
		return -1;
	}


	return 0;
}

int tcp_connect(SOCKET* s, u_long remote, u_short port)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = remote;
	addr.sin_port = htons(port);

	connect(*s, (struct sockaddr*)&addr, sizeof(addr));

	return 0;
}

int tcp_send(SOCKET* s, void* buf)
{
	int ret = send(*s, (char*)buf, strlen((char*)buf), 0);
	if(ret == SOCKET_ERROR)
	{
		fprintf(stdout, "send failed: %d\n", WSAGetLastError());
		closesocket(*s);
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
		closesocket(*s);
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
		closesocket(*s);
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

	ret = recvfrom(*s, (char*)buf, UDP_MSG_MAX_LEN, 0, (struct sockaddr*)&addr, &addrsize);
	if(ret == SOCKET_ERROR)
	{
		fprintf(stdout, "recvfrom failed: %d\n", WSAGetLastError());
		if(WSAGetLastError() != 10040)
		{
			closesocket(*s);
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