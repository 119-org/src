#include <stdio.h>
#include <stdlib.h>
#include "libsocket.h"

int main(int argc, char **argv)
{
	char ip_list[2][64] = {0};
	int num;
	int i;

	num = sock_get_host_list(ip_list, 2);
//	num = sock_get_host_list_byname(ip_list, "www.sina.com", 2);
	for (i = 0; i < 2; i++) {
		printf("ip = %s\n", ip_list[i]);
	}

	return 0;
}
