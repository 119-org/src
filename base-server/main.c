
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "libtcpip.h"

//Normal, thread increases if more than 10 connects per thread;
//therefore, every thread increases connect when threads number reach to MAX_THREAD
#define	MAX_THREAD_NUM			20
#define NORMAL_CONNECTS_PER_THREAD	10

struct srv_param_t {
	SOCKET sock_s;
	int listen_port;	
	int max_client;
	int timeout;
};

struct thread_param_t {
	pthread_t thread_id;	
	SOCKET sock_accept;
};

struct thread_ctl_t {
	pthread_t thread_id;
	pthread_mutex_t thread_lock;
	
};

pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;

static void thread_init()
{
	int i;
	pthread_t id;
	struct thread_ctl_t* thread_list;
	thread_list = (struct thread_ctl_t*)malloc(sizeof(struct thread_ctl_t) * MAX_THREAD_NUM);
	memset(thread_list, 0, sizeof(*thread_list) * MAX_THREAD_NUM);

	for (i = 0; i < MAX_THREAD_NUM; i++) {
		pthread_mutex_init(thread_list[i].thread_lock, NULL);
		pthread_mutex_lock(&thread_list[i].thread_lock);
		ret = pthread_create(&id, NULL, thread_foo, (void*)thread_list[i]);
		pthread_mutex_unlock(&thread_list[i].thread_lock);
		thread_list[i].thread_id = id;

	}



}
static void thread_add(struct thread_ctl_t* ctl)
{
	int ret;
	pthread_t id;
	pthread_mutex_lock(&ctl->thread_lock);
	ret = pthread_create(&id, NULL, thread_foo, (void*)&ctl);
	pthread_mutex_unlock(&ctl->thread_lock);

	ctl->thread_id = id;
	
}
static void thread_del()
{
}
int thread_num = 0;
static void *thread_foo(void* arg)
{
	struct thread_param_t *param = (struct thread_param_t *)arg;
	SOCKET sock_accept = param->sock_accept;
	printf("sock_accept = %d\n", sock_accept);
	int len =  0;
	char mem[1024] = {0};
	thread_num++;
	printf("%d thread_foo: ", thread_num);

	do {
		len = read(sock_accept, mem, sizeof(mem));
		printf("receive %d bytes: '%s'\n", len, mem);
	} while (len < 0);

	return;
	
}

struct tcp_server_t {
	int listen_port;
	SOCKET sock;
	SOCKET sock_accept;
	};

int network_init(SOCKET* sock)
{
	int listen_port = 5060;

	ret = socket_init();
	ret = tcp_socket(&sock);
	ret = tcp_bind(&sock, listen_port);
	ret = tcp_listen(&sock);
}

int main(int argc, char* argv[])
{
	int ret;
	SOCKET sock;
	SOCKET sock_accept;
	struct sockaddr_in client;
	
	network_init(&sock);
	int len = 0;
	char mem[100] = {0};

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
				break;
			default:
				printf("selset ret = %d\n", ret);
				if (FD_ISSET(sock, &rfds)) {
					tcp_accept(&sock, &client, &sock_accept);

					printf("one tcp client comming, ip =  sock_accept = %d\n", sock_accept);
					pthread_t thread_id;
					struct thread_param_t *thread_arg = NULL;
					thread_arg = (struct thread_param_t*)malloc(sizeof(struct thread_param_t));
					memset(thread_arg, 0, sizeof(struct thread_param_t));
					thread_arg->sock_accept = sock_accept; 
					ret = pthread_create(&thread_id, NULL, thread_foo, (void*)thread_arg);
					//check thread/fork is idle?
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
