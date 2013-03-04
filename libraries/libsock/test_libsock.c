#include <stdio.h>
#include <stdlib.h>
#include "libsock.h"

#define IP_LIST		6

int main(int argc, char **argv)
{
    char ip_list[IP_LIST][64] = { 0 };
    int num;
    int i;

    num = sock_get_host_list(ip_list, IP_LIST);
//  num = sock_get_host_list_byname(ip_list, "www.sina.com", 2);
    for (i = 0; i < IP_LIST; i++) {
        printf("ip = %s\n", ip_list[i]);
    }
    printf("list num = %d\n", num);

    return 0;
}
