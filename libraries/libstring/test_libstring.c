
#include "libstring.h"


void foo()
{
    struct string *tmp, *tmp2, *tmp3;
    tmp = str_init();
    tmp2 = str_init();
    tmp3 = str_init();

    str_nchar(tmp, 4, 'b');
    str_nchar(tmp2, 10, 'a');
    str_set(tmp3, "xxx");
    str_push_back(tmp3, 'a');
    str_append(tmp3, "appendd");
    str_insert(tmp3, 10, "insert");
    printf("addr = %d, tmp = %s, len = %d\n", str_addr(tmp), str_data(tmp), str_length(tmp));
    printf("addr = %d, tmp2 = %s, len = %d\n", str_addr(tmp2), str_data(tmp2), str_length(tmp2));
    printf("addr = %d, tmp2 = %s, len = %d\n", str_addr(tmp3), str_data(tmp3), str_length(tmp3));

    str_deinit(tmp);
    str_deinit(tmp2);
    str_deinit(tmp3);
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
