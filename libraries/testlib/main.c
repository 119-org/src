
#include <stdlib.h>

enum PRINT_LEVEL {
	LOG = 0,
	DBG,
	WAR,
	ERR
};

#define	PROMPT		"testlib>"

void dbg_printf(SIGN32 logLevel, const char* string, ...)
{
	static char dbg_buf[256];
	va_list args;
	
	va_start(args, string);
	vsprintf(dbg_buf, string, args);
	va_end(args);
}

void show_prompt(char* prompt)
{
	dbg_printf(LOG, "\n%s", prompt);
}

int main()
{

	while (1) {
		show_prompt(PROMPT);
		
		}
	
	return 0;
}
