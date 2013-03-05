
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "libmisc.h"

#if defined(_MSC_VER) || defined(_WIN32)
unsigned long get_time_ms(void)
{
	DWORD ms;
	ms = GetTickCount();
	return ms;
}
#else
unsigned long get_time_ms(void)
{
	struct timeval tv;
	unsigned long ms;

	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	return ms;
}
#endif

