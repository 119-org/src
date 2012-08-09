
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define	PROMPT		"testlib>"
unsigned int g_exit;

enum PRINT_LEVEL {
	LOG = 0,
	DBG,
	WAR,
	ERR
};

enum g_cmd_set {
	CMD_EXIT,
	CMD_HELP,
	CMD_MAX,
	CMD_UNKNOWN = 100
};
const char* g_cmd_set_str[] = {
	"EXIT",
	"HELP"	
};

const char* g_cmd_help[] = {
	"EXIT",
	"[]; help",
	"Usage: CMD <param0> <param1> ...\n"
};

void dbg_printf(int logLevel, const char* string, ...)
{
	static char dbg_buf[256];
	va_list args;
	
	va_start(args, string);
	vprintf(string, args);
//	printf("%s", dbg_buf);
	va_end(args);
}

void print_help(const char** cmd, const char** help, unsigned int max)
{
	int i;
	for (i = 0; i < max; i++) {
		dbg_printf(LOG, "%s\t%s\n", cmd[i], help[i]);
	}
}
void print_prompt(char* prompt)
{
	dbg_printf(LOG, "%s", prompt);
}

void skip_space(char** str)
{
	char* s = *str;
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	*str = s;
}
int get_elem_len(char* s)
{
	char tmp;
	int i;
	for (i = 0; s[i] != 0; i++) {
		tmp = s[i];
		if (tmp == ' ' || tmp == '\t' || tmp == '\r' || tmp == '\n')
			break;
	}
	return i;
}
int find_cmd_index(char** str, const char** cmd_table, int cmd_max)
{
	int i;
	int elem_len;
	char* s = *str;
	char* tmp;

	elem_len = get_elem_len(s);
	if (elem_len == 0)
		return CMD_UNKNOWN;
	
	for (i = 0; i < cmd_max; i++) {
		tmp = (char*)cmd_table[i];
		if (elem_len != strlen(tmp))
			continue;
		if (strncmp(s, tmp, elem_len) == 0)
			break;
	}

	if (i == cmd_max) {
		return CMD_UNKNOWN;
	}

	*str = &s[elem_len];
	return i;
}

int parse_cmd(char* cmd)
{
	int cmd_index;

	skip_space(&cmd);

	if (*cmd == '\0' || *cmd == ';')
		return 1;
	
	cmd_index = find_cmd_index(&cmd, g_cmd_set_str, CMD_MAX);

	dbg_printf(LOG, "find_cmd_index: %d\n", cmd_index);
	switch (cmd_index) {
		case CMD_EXIT:
			g_exit = 1;
			break;
		case CMD_HELP:
		default:
			print_help(g_cmd_set_str, g_cmd_help, CMD_MAX);
			break;
	}
}
int main()
{
	char* cmd;
	size_t str_len;

	g_exit = 0;

	while (g_exit != 1) {
		print_prompt(PROMPT);
		getline(&cmd, &str_len, stdin);
		parse_cmd(cmd);	
//		free(cmd);
	}
	
	return 0;
}
