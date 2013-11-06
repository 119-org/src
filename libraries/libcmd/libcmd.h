#ifndef _LIBCMD_H_
#define _LIBCMD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cmd_tbl {
    const char *name;   /* Command Name */
    int maxargs;        /* maximum number of arguments */
    int repeatable;     /* autorepeat allowed? */
    int (*cmd)(struct cmd_tbl *, int, int, char *const[]);
    const char *usage;	/* Usage message  (short) */
    const char *help;	/* Help  message  (long)  */
} cmd_t;

typedef int (*cmd_cb)(cmd_t *, int, int, char *const[]);

extern void cmd_init();
extern void cmd_add(const char *name, int maxargs, int repeatable, const char *usage, const char *help, cmd_cb cmd);
extern void cmd_loop();

#ifdef __cplusplus
}
#endif
#endif
