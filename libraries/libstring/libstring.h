#ifndef _LIBSTRING_H_
#define _LIBSTRING_H_

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct string {
    void *buf;
    size_t len;
};

struct string *str_init();
void str_deinit();
void str_set(struct string *s, const char *str);
char* str_data(struct string *s);
size_t str_size();
size_t str_length(struct string *s);
void str_push_back(struct string *s, char c);
void str_append(struct string *s, const char *str);
void str_insert(struct string *s, size_t pos, const char *str);
void str_nchar(struct string *s, size_t n, char c);
size_t str_addr(struct string *s);


#ifdef __cplusplus
}
#endif

#endif
