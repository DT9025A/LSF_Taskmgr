#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- string helpers ---- */
char *xstrndup(const char *s, size_t n) {
    char *r = (char *)malloc(n + 1);
    if (!r) return NULL;
    memcpy(r, s, n);
    r[n] = 0;
    return r;
}
char *xstrdup(const char *s) { return s ? xstrndup(s, strlen(s)) : NULL; }

/* ---- string buffer ---- */
void sb_init(StrBuf *sb) { sb->data = NULL; sb->len = sb->cap = 0; }

void sb_append(StrBuf *sb, const char *s, size_t n) {
    if (sb->len + n + 1 > sb->cap) {
        size_t nc = sb->cap ? sb->cap : 1024;
        while (nc < sb->len + n + 1) nc *= 2;
        sb->data = (char *)realloc(sb->data, nc);
        sb->cap = nc;
    }
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = 0;
}
void sb_puts(StrBuf *sb, const char *s) { sb_append(sb, s, strlen(s)); }
void sb_free(StrBuf *sb) { free(sb->data); sb_init(sb); }

/* ---- external commands / files ---- */
char *run_cmd(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    StrBuf sb; sb_init(&sb);
    if (fp) {
        char buf[4096]; size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) sb_append(&sb, buf, n);
        pclose(fp);
    }
    if (!sb.data) sb_puts(&sb, "");
    return sb.data;
}

char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    StrBuf sb; sb_init(&sb);
    char buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) sb_append(&sb, buf, n);
    fclose(f);
    if (!sb.data) sb_puts(&sb, "");
    return sb.data;
}

/* ---- line splitting ---- */
char **split_lines(const char *s, int *out_n) {
    if (!s) { *out_n = 0; return NULL; }
    char **res = NULL; int n = 0, cap = 0;
    const char *p = s;
    while (*p) {
        const char *start = p;
        while (*p && *p != '\n') p++;
        size_t len = (size_t)(p - start);
        while (len > 0 && start[len - 1] == '\r') len--;
        if (n >= cap) { cap = cap ? cap * 2 : 32;
            res = (char **)realloc(res, cap * sizeof(char *)); }
        res[n++] = xstrndup(start, len);
        if (*p == '\n') p++;
    }
    *out_n = n;
    return res;
}

void free_strs(char **arr, int n) {
    if (!arr) return;
    for (int i = 0; i < n; i++) free(arr[i]);
    free(arr);
}

char **split_ws(const char *line, int *out_n) {
    char **res = NULL; int n = 0, cap = 0;
    const char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (n >= cap) { cap = cap ? cap * 2 : 16;
            res = (char **)realloc(res, cap * sizeof(char *)); }
        res[n++] = xstrndup(start, (size_t)(p - start));
    }
    *out_n = n;
    return res;
}

/* ================================================================
 *  Line array
 * ================================================================ */

void la_init(LineArray *la) {
    la->items = NULL;
    la->count = la->cap = 0;
}

void la_free(LineArray *la) {
    for (int i = 0; i < la->count; i++) {
        Line *l = &la->items[i];
        for (int s = 0; s < l->nsegs; s++) free(l->segs[s].text);
        free(l->segs);
        free(l->job_id);
    }
    free(la->items);
    la_init(la);
}

/* ---- internal helpers (static, defined before first use) ---- */

static Line *la_new_line(LineArray *la) {
    if (la->count >= la->cap) {
        la->cap = la->cap ? la->cap * 2 : 128;
        la->items = (Line *)realloc(la->items, la->cap * sizeof(Line));
    }
    Line *l = &la->items[la->count++];
    memset(l, 0, sizeof(*l));
    return l;
}

static void line_append_seg(Line *l, const char *text, int color) {
    if (l->nsegs >= l->cap) {
        l->cap = l->cap ? l->cap * 2 : 2;
        l->segs = (Segment *)realloc(l->segs, l->cap * sizeof(Segment));
    }
    l->segs[l->nsegs].text  = xstrdup(text);
    l->segs[l->nsegs].color = color;
    l->nsegs++;
}

/* ---- single-segment API ---- */

void la_add(LineArray *la, const char *text, int color) {
    Line *l = la_new_line(la);
    line_append_seg(l, text, color);
}

void la_addf(LineArray *la, int color, const char *fmt, ...) {
    char buf[8192];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    Line *l = la_new_line(la);
    line_append_seg(l, buf, color);
}

void la_add_job(LineArray *la, const char *text, int color, const char *job_id) {
    Line *l = la_new_line(la);
    line_append_seg(l, text, color);
    l->job_id = job_id ? xstrdup(job_id) : NULL;
}

void la_addf_job(LineArray *la, int color, const char *job_id,
                 const char *fmt, ...) {
    char buf[8192];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    Line *l = la_new_line(la);
    line_append_seg(l, buf, color);
    l->job_id = job_id ? xstrdup(job_id) : NULL;
}

/* ---- multi-segment API ---- */

void la_add_segs(LineArray *la, const SegSpec *segs, int nsegs,
                 const char *job_id) {
    Line *l = la_new_line(la);
    for (int i = 0; i < nsegs; i++)
        line_append_seg(l, segs[i].text, segs[i].color);
    l->job_id = job_id ? xstrdup(job_id) : NULL;
}


