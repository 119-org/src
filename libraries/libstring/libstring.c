#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "libstring.h"

void str_append(struct string *s, const char *str)
{
    int i;
    assert(s);
    for (i = 0; str[i] != '\0'; i++) {
        *((char *)s->buf + s->len) = str[i];
        s->len++;
    }
}

void str_insert(struct string *s, size_t pos, const char* str)
{
    int i, j, len;
    void *tmp;
    assert(s);
    if (pos >= s->len) {
        return str_append(s, str);
    }
    len = s->len + strlen(str);
    tmp = calloc(len, 1);
    for (i = 0; i < pos; i++) {
        *((char *)tmp + i) = *((char *)s->buf + i);
    }
    for (j = 0; str[j] != '\0'; j++, i++) {
        *((char *)tmp + i) = str[j];
    }
    for (j = pos; j < s->len; j++, i++) {
        *((char *)tmp +i) = *((char *)s->buf + j);
    }
    free(s->buf);
    s->buf = tmp;
    s->len = len;
}

void str_set(struct string *s, const char *str)
{
    assert(s);
    s->len = strlen(str);
    s->buf = calloc(s->len, 1);
    strncpy(s->buf, str, s->len);
}

char *str_data(struct string *s)
{
    assert(s);
    return (char *)s->buf;
}

size_t str_size()
{
    return sizeof(struct string);
}

size_t str_addr(struct string *s)
{
    assert(s);
    return (size_t)s->buf;
}

size_t str_length(struct string *s)
{
    size_t i;
    assert(s);
    for (i = 0; *((char *)s->buf + i) != '\0'; ++i)
        ;
    return i;
}

void str_push_back(struct string *s, char c)
{
    assert(s);
    *((char *)s->buf + s->len) = c;
    s->len++;
    *((char *)s->buf + s->len) = '\0';
}

void str_nchar(struct string *s, size_t n, char c)
{
    size_t i;
    assert(s);
    s->buf = calloc(1, n);
    for (i = 0; i < n; ++i) {
        *((char *)s->buf + i) = c;
    }
    s->len = i;
}

struct string *str_init()
{
    struct string *s;
    s = (struct string *)calloc(1, sizeof(struct string));
    assert(s);
    s->buf = NULL;
    s->len = 0;
    return s;
}

void str_deinit(struct string *s)
{
    assert(s);
    free(s);
}
