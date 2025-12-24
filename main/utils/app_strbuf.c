#include "app_strbuf.h"

#include <stdio.h>
#include <string.h>

void app_strbuf_init(app_strbuf_t *sb, char *buf, size_t cap) {
    sb->buf = buf;
    sb->cap = cap;
    sb->len = 0;

    if (cap == 0 || buf == NULL) return;

    buf[0] = '\0';
}

int app_strbuf_append(app_strbuf_t *sb, const char *s) {
    if (sb == NULL || sb->buf == NULL || sb->cap == 0 || s == NULL) return -1;
    return app_strbuf_appendf(sb, "%s", s);
}

int app_strbuf_vappendf(app_strbuf_t *sb, const char *fmt, va_list ap) {
    if (sb == NULL || sb->buf == NULL || sb->cap == 0 || fmt == NULL) return -1;
    if (sb->len >= sb->cap) return 0;

    size_t remaining = sb->cap - sb->len;
    int written = vsnprintf(sb->buf + sb->len, remaining, fmt, ap);
    if (written < 0) {
        // keep buffer NUL-terminated as best effort
        sb->buf[sb->cap - 1] = '\0';
        return -1;
    }

    // vsnprintf returns the number of chars that would have been written excluding '\0'
    size_t advanced = (size_t)written;
    if (advanced >= remaining) {
        sb->len = sb->cap - 1;
        sb->buf[sb->len] = '\0';
        return (int)(remaining - 1);
    }

    sb->len += advanced;
    return written;
}

int app_strbuf_appendf(app_strbuf_t *sb, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int res = app_strbuf_vappendf(sb, fmt, ap);
    va_end(ap);
    return res;
}

