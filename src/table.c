#include "table.h"
#include "util.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void table_free(Table *t) {
    if (t->headers) {
        for (int i = 0; i < t->nheaders; i++) free(t->headers[i]);
        free(t->headers);
    }
    for (int i = 0; i < t->nrows; i++) {
        for (int j = 0; j < t->rowlens[i]; j++) free(t->rows[i][j]);
        free(t->rows[i]);
    }
    free(t->rows);
    free(t->rowlens);
    memset(t, 0, sizeof(*t));
}

Table parse_table(char **lines, int nlines) {
    Table t; memset(&t, 0, sizeof(t));

    int hi = -1;
    for (int i = 0; i < nlines; i++) {
        const char *p = lines[i];
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p) { hi = i; break; }
    }
    if (hi < 0) return t;
    if (strstr(lines[hi], "No unfinished job found")) return t;

    t.headers = split_ws(lines[hi], &t.nheaders);

    int cap = 0;
    for (int i = hi + 1; i < nlines; i++) {
        int nf;
        char **f = split_ws(lines[i], &nf);
        if (nf == 0) { free_strs(f, nf); continue; }

        /* Merge overflow fields into the last column (JOB_NAME may
         * contain spaces in some LSF versions). */
        if (nf > t.nheaders) {
            StrBuf sb; sb_init(&sb);
            for (int k = t.nheaders - 1; k < nf; k++) {
                if (k > t.nheaders - 1) sb_puts(&sb, " ");
                sb_puts(&sb, f[k]);
            }
            free(f[t.nheaders - 1]);
            f[t.nheaders - 1] = sb.data;
            for (int k = t.nheaders; k < nf; k++) free(f[k]);
            nf = t.nheaders;
        }
        if (t.nrows >= cap) {
            cap = cap ? cap * 2 : 16;
            t.rows    = (char ***)realloc(t.rows,    cap * sizeof(char **));
            t.rowlens = (int *)   realloc(t.rowlens, cap * sizeof(int));
        }
        t.rows[t.nrows]    = f;
        t.rowlens[t.nrows] = nf;
        t.nrows++;
    }
    return t;
}

int table_col(Table *t, const char *name) {
    for (int i = 0; i < t->nheaders; i++)
        if (strcmp(t->headers[i], name) == 0) return i;
    return -1;
}

const char *table_get(Table *t, int r, int c) {
    if (r < 0 || r >= t->nrows || c < 0) return "";
    if (c >= t->rowlens[r]) return "";
    return t->rows[r][c];
}
