#include <stdio.h>
#include "libcmd.h"

int do_ls(cmd_t *cmdtp, int ptr, int argc, char *const argv[])
{

    return 0;
}

int main(int argc, char **argv)
{
    cmd_init();
    cmd_add("ls", 5, 0, "ls_file usage", "ls_file from dir", do_ls);

    cmd_loop();

    return 0;
}
