/* Port of jobloca.py's parsing logic. */
#include "jobinfo.h"
#include "util.h"
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- quotes ---- */
char *strip_quotes(const char *v) {
    size_t n = strlen(v);
    if (n >= 2 && v[0] == '"' && v[n - 1] == '"') return xstrndup(v + 1, n - 2);
    return xstrdup(v);
}

/* ---- script path ---- */
char *extract_script_path(const char *bjobs_out) {
    const char *pats[] = {
        "/simulation/[^ \t\n\r]*runICRP[0-9]+",
        "/simulation/[^ \t\n\r]*swiftJob[0-9]+",
        "/simulation/[^ \t\n\r]*runGenricJob[0-9]+"
    };
    for (int i = 0; i < 3; i++) {
        regex_t re;
        if (regcomp(&re, pats[i], REG_EXTENDED) != 0) continue;
        regmatch_t m[1];
        if (regexec(&re, bjobs_out, 1, m, 0) == 0) {
            char *r = xstrndup(bjobs_out + m[0].rm_so,
                               (size_t)(m[0].rm_eo - m[0].rm_so));
            regfree(&re);
            return r;
        }
        regfree(&re);
    }
    return NULL;
}

/* ---- cd path ---- */
char *get_cd_path(const char *script) {
    int n; char **lines = split_lines(script, &n);
    char *res = NULL;
    for (int i = 0; i < n; i++) {
        const char *p = lines[i];
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "cd ", 3) == 0) {
            int nf; char **f = split_ws(p, &nf);
            if (nf >= 2) { res = xstrdup(f[1]); free_strs(f, nf); break; }
            free_strs(f, nf);
        }
    }
    free_strs(lines, n);
    return res;
}

/* ---- runICRP ---- */
void extract_runICRP_params(const char *content,
                            char **mpssession, char **logf, char **mpshost) {
    *mpssession = *logf = *mpshost = NULL;
    int n; char **lines = split_lines(content, &n);
    for (int i = 0; i < n; i++) {
        const char *p = lines[i];
        while (*p == ' ' || *p == '\t') p++;
        int ok = 0;
        if (strncmp(p, "virtuoso", 8) == 0) ok = 1;
        else if (*p == '/') {
            const char *e = p;
            while (*e && *e != ' ' && *e != '\t') e++;
            char *tok = xstrndup(p, (size_t)(e - p));
            if (strstr(tok, "virtuoso")) ok = 1;
            free(tok);
        }
        if (!ok) continue;
        int nt; char **toks = split_ws(p, &nt);
        for (int k = 0; k < nt - 1; k++) {
            if      (!strcmp(toks[k], "-mpssession")) { free(*mpssession); *mpssession = strip_quotes(toks[k+1]); }
            else if (!strcmp(toks[k], "-log"))        { free(*logf);      *logf      = strip_quotes(toks[k+1]); }
            else if (!strcmp(toks[k], "-mpshost"))    { free(*mpshost);   *mpshost   = strip_quotes(toks[k+1]); }
        }
        free_strs(toks, nt);
        break;
    }
    free_strs(lines, n);
}

/* ---- swiftJob ---- */
void extract_swiftJob_params(const char *content,
                             char **hn, char **bh, char **sl,
                             char **sc, char **sv) {
    *hn = *bh = *sl = *sc = *sv = NULL;
    int n; char **lines = split_lines(content, &n);
    for (int i = 0; i < n; i++) {
        const char *p = lines[i];
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "/software/app/cadence", 21) != 0) continue;
        int nt; char **toks = split_ws(p, &nt);
        for (int k = 0; k < nt - 1; k++) {
            if      (!strcmp(toks[k], "--historyName")) { free(*hn); *hn = strip_quotes(toks[k+1]); }
            else if (!strcmp(toks[k], "-beanhost"))     { free(*bh); *bh = strip_quotes(toks[k+1]); }
            else if (!strcmp(toks[k], "--sdbLib"))      { free(*sl); *sl = strip_quotes(toks[k+1]); }
            else if (!strcmp(toks[k], "--sdbCell"))     { free(*sc); *sc = strip_quotes(toks[k+1]); }
            else if (!strcmp(toks[k], "--sdbView"))     { free(*sv); *sv = strip_quotes(toks[k+1]); }
        }
        free_strs(toks, nt);
        break;
    }
    free_strs(lines, n);
}

/* ---- Library / Cell / State from log ---- */
void extract_lib_cell_state(const char *log_path,
                            char **lib, char **cell, char **state) {
    *lib = *cell = *state = NULL;
    if (!log_path) return;
    char *content = read_file(log_path);
    if (!content) return;

    regex_t re; regmatch_t m[2];
    const char *pats[3] = {
        "Library[ \t]*=[ \t]*(.*)",
        "Cell[ \t]*=[ \t]*(.*)",
        "State Name[ \t]*=[ \t]*(.*)"
    };
    char **outs[3] = { lib, cell, state };
    for (int i = 0; i < 3; i++) {
        if (regcomp(&re, pats[i], REG_EXTENDED) != 0) continue;
        if (regexec(&re, content, 2, m, 0) == 0) {
            const char *p = content + m[1].rm_so;
            size_t len = (size_t)(m[1].rm_eo - m[1].rm_so);
            while (len > 0 && (p[len-1] == ' ' || p[len-1] == '\t' ||
                               p[len-1] == '\r' || p[len-1] == '\n')) len--;
            *outs[i] = xstrndup(p, len);
        }
        regfree(&re);
    }
    free(content);
}

/* ---- job id / log path ---- */
char *extract_job_id_from_script_path(const char *p) {
    regex_t re;
    if (regcomp(&re, "swiftJob([0-9]+)", REG_EXTENDED) != 0) return NULL;
    regmatch_t m[2];
    char *res = NULL;
    if (regexec(&re, p, 2, m, 0) == 0)
        res = xstrndup(p + m[1].rm_so, (size_t)(m[1].rm_eo - m[1].rm_so));
    regfree(&re);
    return res;
}

char *find_and_resolve_log_path(const char *content, const char *job_id) {
    regex_t re;
    if (regcomp(&re,
                "CDS_SIM_MONITOR_LOG_FILE[ \t]*=[ \t]*\"([^\"]*)\"",
                REG_EXTENDED) != 0)
        return NULL;
    regmatch_t m[2];
    char *res = NULL;
    if (regexec(&re, content, 2, m, 0) == 0) {
        char *raw = xstrndup(content + m[1].rm_so,
                             (size_t)(m[1].rm_eo - m[1].rm_so));
        StrBuf sb; sb_init(&sb);
        const char *p = raw;
        const char *needle = "${COLLECTION_JOB_ID}";
        size_t nlen = strlen(needle);
        while (*p) {
            if (strncmp(p, needle, nlen) == 0) {
                sb_puts(&sb, job_id ? job_id : "");
                p += nlen;
            } else {
                sb_append(&sb, p, 1);
                p++;
            }
        }
        res = sb.data ? sb.data : xstrdup("");
        free(raw);
    }
    regfree(&re);
    return res;
}

/* ---- JSON ---- */
char *json_get(const char *line, const char *key) {
    char pat[256];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(line, pat);
    if (!p) return NULL;
    p += strlen(pat);
    while (*p && *p != ':') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t') p++;

    if (*p == '"') {
        p++;
        const char *e = p;
        while (*e && *e != '"') { if (*e == '\\' && *(e+1)) e++; e++; }
        return xstrndup(p, (size_t)(e - p));
    }
    const char *e = p;
    while (*e && *e != ',' && *e != '}') e++;
    while (e > p && (e[-1] == ' ' || e[-1] == '\t')) e--;
    return xstrndup(p, (size_t)(e - p));
}

int parse_last_json_record(const char *log_path,
                           char **history, char **point,
                           char **testname, char **cornername) {
    *history = *point = *testname = *cornername = NULL;
    char *content = read_file(log_path);
    if (!content) return 0;
    int n; char **lines = split_lines(content, &n);
    int found = 0;
    for (int i = n - 1; i >= 0; i--) {
        if (!strchr(lines[i], '{')) continue;
        char *h = json_get(lines[i], "history");
        char *p = json_get(lines[i], "point");
        char *t = json_get(lines[i], "testname");
        char *c = json_get(lines[i], "cornername");
        if (h && p && t && c) {
            *history = h; *point = p; *testname = t; *cornername = c;
            found = 1; break;
        }
        free(h); free(p); free(t); free(c);
    }
    free_strs(lines, n);
    free(content);
    return found;
}

/* ---- runGenricJob path ---- */
int extract_runGenricJob_info(const char *path, GenericInfo *out) {
    memset(out, 0, sizeof(*out));
    int cap = 0, n = 0;
    char **parts = NULL;
    const char *p = path;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != '/') p++;
        if (n >= cap) { cap = cap ? cap * 2 : 16;
            parts = (char **)realloc(parts, cap * sizeof(char *)); }
        parts[n++] = xstrndup(start, (size_t)(p - start));
    }
    int job_idx = -1;
    for (int i = 0; i < n; i++)
        if (strstr(parts[i], "runGenricJob")) { job_idx = i; break; }
    if (job_idx < 9) { free_strs(parts, n); return 0; }

    out->library      = xstrdup(parts[job_idx - 9]);
    out->cell         = xstrdup(parts[job_idx - 8]);
    out->simview      = xstrdup(parts[job_idx - 7]);
    out->history_view = xstrdup(parts[job_idx - 4]);

    const char *skip = (job_idx - 3) >= 0 ? parts[job_idx - 3] : "";
    if (strlen(skip) == 1 && isdigit((unsigned char)skip[0]))
        out->testname = xstrdup(parts[job_idx - 2]);
    else
        out->testname = xstrdup(parts[job_idx - 3]);

    free_strs(parts, n);
    return 1;
}
void free_generic(GenericInfo *g) {
    free(g->library); free(g->cell); free(g->simview);
    free(g->history_view); free(g->testname);
}
