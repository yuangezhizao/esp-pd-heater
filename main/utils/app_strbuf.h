#pragma once

#include <stdarg.h>
#include <stddef.h>

typedef struct {
    char *buf;
    size_t cap;  // total buffer size, including trailing '\0'
    size_t len;  // current string length (excluding trailing '\0')
} app_strbuf_t;

void app_strbuf_init(app_strbuf_t *sb, char *buf, size_t cap);
int app_strbuf_append(app_strbuf_t *sb, const char *s);
int app_strbuf_appendf(app_strbuf_t *sb, const char *fmt, ...);
int app_strbuf_vappendf(app_strbuf_t *sb, const char *fmt, va_list ap);

