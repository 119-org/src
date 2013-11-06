#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "libcmd.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define CMD_SYS_CONSOLE_LEN	1024        /* Console I/O Buffer Size */
#define CMD_MAX_ARGS		16          /* max number of command args */
#define CMD_MAX_ARGS		16

#define CMD_PROMPT	"cmd> "
#define CMD_MAX_LEN	50

static cmd_t g_cmd_tbl[CMD_MAX_LEN + 1];
static cmd_t *g_cmd_tbl_start = g_cmd_tbl;
static cmd_t *g_cmd_tbl_end = (g_cmd_tbl + CMD_MAX_LEN);
static char *g_cmd_name[CMD_MAX_LEN + 1];

static int cmd_len(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(g_cmd_tbl); i++)
        if (g_cmd_tbl[i].name == NULL)
            break;
    return i;
}

static void cmd_tbl_init(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(g_cmd_tbl); i++) {
        g_cmd_tbl[i].name = NULL;
        g_cmd_tbl[i].cmd = NULL;
        g_cmd_tbl[i].usage = NULL;
        g_cmd_tbl[i].help = NULL;
        g_cmd_tbl[i].maxargs = 0;
        g_cmd_tbl[i].repeatable = 0;
    }
}

static char *str_find(const char *s, int c)
{
    for (; *s != (char)c; ++s)
        if (*s == '\0')
            return NULL;
    return (char *)s;
}

static cmd_t *find_cmd(const char *cmd)
{
    cmd_t *cmdtp;
    const char *p;
    unsigned int len;

    if ((p = str_find(cmd, '-')) == NULL) {
        len = strlen(cmd);
    } else {
        len = p - cmd;

    }

    for (cmdtp = g_cmd_tbl_start; cmdtp != g_cmd_tbl_end; cmdtp++) {
        if (cmdtp->name == NULL)
            return NULL;
        if (strncmp(cmd, cmdtp->name, len) == 0) {
            if (len == strlen(cmdtp->name)) {
                return cmdtp;   /* full match */
            }
        }
    }
    return NULL;                /* not found or ambiguous command */
}

static int parse_line(char *line, char *argv[], char *point_flag)
{
    int nargs = 0;
    char last = 0;

    while (nargs < CMD_MAX_ARGS) {
        /* skip any white space */
        while ((*line == ' ') || (*line == '\t') || (*line == '-')) {
            last = *line;
            ++line;
        }
        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = NULL;
            return (nargs);
        }
        argv[nargs++] = line;   /* begin of argument string    */
        /* find end of string */
        while (*line && (*line != ' ') && (*line != '\t')) {
            if (*line == '-') {
                if (last != ' ') {
                    last = *line;
                    ++line;
                } else {
                    break;
                }
            } else {
                last = *line;
                ++line;
            }
        }
        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = NULL;
            return (nargs);
        }
        if (*line == '-') {
            *point_flag = 1;
        }
        *line++ = '\0';         /* terminate current arg          */
    }
    printf("** Too many args (max. %d) **\n", CMD_MAX_ARGS);
    return (nargs);
}

static int cmd_usage(cmd_t * cmdtp)
{
    printf("%s - %s\n\n", cmdtp->name, cmdtp->usage);
    printf("Usage:\n%s ", cmdtp->name);
    if (!cmdtp->help) {
        printf("- No additional help available.\n");
        return -1;
    }
    printf("%s\n", cmdtp->help);
    printf("\n");
    return 1;
}

static int cmd_run(char *cmd)
{
    char point_flag = 0;
    cmd_t *cmdtp;
    char *argv[CMD_MAX_ARGS + 1];
    int argc;

    if (!cmd || !*cmd) {
        return -1;
    }

    if (strlen(cmd) >= CMD_SYS_CONSOLE_LEN) {
        printf("## Command too long!\n");
        return -1;
    }

    if ((argc = parse_line(cmd, argv, &point_flag)) == 0) {
        return -1;              /* no command at all */
    }
    if ((cmdtp = find_cmd(argv[0])) == NULL) {
        printf("Unknown command %s,try 'help %s'\n", cmd, cmd);
        return -1;
    }

    if (cmdtp == NULL)
        return -1;
    if (argc > cmdtp->maxargs) {
        cmd_usage(cmdtp);
        return -1;
    }
    if ((cmdtp->cmd) (cmdtp, point_flag, argc, argv) != 0) {
        printf("run do_cmd error\n");
    }
    return 0;
}

int do_help(cmd_t * cmdtp, int point_flag, int argc, char *const argv[])
{
    int i;
    printf("libcmd bash version 0.1\n");
    for (i = 1; i < cmd_len(); i++) {
        printf("%s\n", g_cmd_tbl[i].name);
    }
    return 0;
}

static char *cmd_gen(const char *text, int state)
{
    const char *name;
    static int i, len;

    if (!state) {
        i = 0;
        len = strlen(text);
    }
    while ((name = g_cmd_name[i]) != NULL) {
        i++;

        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }

    return ((char *)NULL);
}

static char **cmd_comp_f(const char *text, int start, int end)
{
    char **matches = NULL;

    if (start == 0)
        matches = rl_completion_matches(text, cmd_gen);
    return (matches);
}

static char *cmd_gets()
{
    char *line_read = (char *)NULL;
    if (line_read) {
        free(line_read);
        line_read = (char *)NULL;
    }
    line_read = readline(CMD_PROMPT);

    if (line_read && *line_read)
        add_history(line_read);
    return (line_read);
}

void cmd_init()
{
    cmd_tbl_init();
    rl_readline_name = "libcmd_sh";
    rl_attempted_completion_function = cmd_comp_f;
    cmd_add("help", 1, 0, "usage:help", "all the cmd show", do_help);
}

void cmd_add(const char *name, int maxargs, int repeatable, const char *usage,
             const char *help, cmd_cb cmd)
{
    int i = cmd_len();
    if (i >= CMD_MAX_LEN)
        return;
    (g_cmd_tbl_start + i)->name = name;
    (g_cmd_tbl_start + i)->maxargs = maxargs;
    (g_cmd_tbl_start + i)->cmd = cmd;
    (g_cmd_tbl_start + i)->repeatable = repeatable;
    (g_cmd_tbl_start + i)->usage = usage;
    (g_cmd_tbl_start + i)->help = help;
    g_cmd_name[i] = (char *)name;
}


void cmd_loop()
{
    char *s;

    while (1) {
        s = cmd_gets();
        cmd_run(s);
    }
}
