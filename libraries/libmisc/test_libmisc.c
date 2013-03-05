
#include "libmisc.h"

int main(int argc, char **argv)
{

	printf("system time %lldms\n", get_time_ms());
	sleep(1);
	printf("system tmie %lldms\n", get_time_ms());
	
	
}
