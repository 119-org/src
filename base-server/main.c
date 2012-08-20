
#include <stdio.h>
#include <stdlib.h>
#include "libtcpip.h"


int main(int argc, char* argv[])
{
	srv_init();
	srv_listen();
	srv_run();

	return 0;	
}
