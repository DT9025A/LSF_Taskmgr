#ifndef LSF_UTIL_H
#define LSF_UTIL_H
#include <stddef.h>

/* ---- string helpers ---- */
char *xstrndup(const char *s, size_t n);
char *xstrdup(const char *s);

/* ---- string buffer ---- */
typedef struct { char *data; size_t len, cap; } StrBuf;
void sb_init(StrBuf *sb);
void sb_append(StrBuf *sb, const char *s, size_t n);
void sb_puts(StrBuf *sb, const char *s);
void sb_free(StrBuf *sb);

/* ---- external commands / files ---- */
char *run_cmd(const char *cmd);
char *read_file(const char *path);

/* ---- line splitting ---- */
char **split_lines(const char *s, int *out_n);
void   free_strs(char **arr, int n);
char **split_ws(const char *line, int *out_n);

/* ---- palette indices ---- */
enum {
    C_BLACK, C_WHITE, C_RED, C_GREEN, C_BLUE,
    C_PURPLE, C_GRAY, C_BAR, C_SEL, C_MAX
};

/* ---- line array ---- */
typedef struct {
    char *text;
    int   color;
} Segment;

typedef struct {
    Segment *segs;
    int      nsegs;
    int      cap;
    char    *job_id;      /* optional; non-NULL if this line represents a job */
} Line;

typedef struct {
    const char *text;
    int         color;
} SegSpec;

typedef struct { Line *items; int count, cap; } LineArray;

/* existing single-segment API (unchanged semantics) */
void la_add     (LineArray *la, const char *text, int color);
void la_addf    (LineArray *la, int color, const char *fmt, ...);
void la_add_job (LineArray *la, const char *text, int color, const char *job_id);
void la_addf_job(LineArray *la, int color, const char *job_id, const char *fmt, ...);

/* new multi-segment API */
void la_add_segs(LineArray *la, const SegSpec *segs, int nsegs, const char *job_id);

void la_init(LineArray *la);
void la_free(LineArray *la);

#endif
