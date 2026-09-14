/* Port of jobrsrc.py, extended to parse all rows of one bjobs -W run. */
#include "resources.h"
#include "table.h"
#include "util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double parse_cpu_time(const char *s) {
    int h, m; double sec;
    if (!s) return 0.0;
    if (sscanf(s, "%d:%d:%lf", &h, &m, &sec) == 3)
        return h * 3600.0 + m * 60.0 + sec;
    return 0.0;
}

time_t parse_start_time(const char *s) {
    int mo, dy, hh, mm, ss;
    if (!s) return 0;
    if (sscanf(s, "%d/%d-%d:%d:%d", &mo, &dy, &hh, &mm, &ss) != 5) return 0;

    time_t now = time(NULL);
    struct tm tm_now; localtime_r(&now, &tm_now);
    struct tm tm_s = tm_now;
    tm_s.tm_mon   = mo - 1;
    tm_s.tm_mday  = dy;
    tm_s.tm_hour  = hh;
    tm_s.tm_min   = mm;
    tm_s.tm_sec   = ss;
    tm_s.tm_isdst = -1;
    time_t t = mktime(&tm_s);
    if (t > now) { tm_s.tm_year -= 1; t = mktime(&tm_s); }
    return t;
}

int count_pids(const char *s) {
    int cnt = 0;
    if (!s) return 0;
    const char *p = s;
    while (*p) {
        while (*p == ',' || *p == ' ') p++;
        if (!*p) break;
        if (isdigit((unsigned char)*p)) cnt++;
        while (*p && *p != ',') p++;
    }
    return cnt;
}

RInfo *extract_all_rinfos(char **lines, int nlines, int *out_n) {
    *out_n = 0;
    Table t = parse_table(lines, nlines);
    if (t.nrows == 0) { table_free(&t); return NULL; }

    int cJ = table_col(&t, "JOBID");
    int cQ = table_col(&t, "QUEUE");
    int cF = table_col(&t, "FROM_HOST");
    int cE = table_col(&t, "EXEC_HOST");
    int cP = table_col(&t, "PIDS");
    int cC = table_col(&t, "CPU_USED");
    int cS = table_col(&t, "START_TIME");

    int n = t.nrows;
    RInfo *arr = (RInfo *)calloc((size_t)n, sizeof(RInfo));
    time_t now = time(NULL);
    for (int r = 0; r < n; r++) {
        arr[r].job_id     = xstrdup(table_get(&t, r, cJ));
        arr[r].queue      = xstrdup(table_get(&t, r, cQ));
        arr[r].from_host  = xstrdup(table_get(&t, r, cF));
        arr[r].exec_host  = xstrdup(table_get(&t, r, cE));
        arr[r].pids_count = count_pids(table_get(&t, r, cP));

        double cpu_sec = parse_cpu_time(table_get(&t, r, cC));
        time_t start   = parse_start_time(table_get(&t, r, cS));
        arr[r].elapsed = start ? (long)(now - start) : 0;
        arr[r].cpu_usage = (arr[r].elapsed > 0)
                         ? cpu_sec / arr[r].elapsed * 100.0 : 0.0;
    }
    table_free(&t);
    *out_n = n;
    return arr;
}

void free_rinfos(RInfo *a, int n) {
    if (!a) return;
    for (int i = 0; i < n; i++) {
        free(a[i].job_id);
        free(a[i].queue);
        free(a[i].from_host);
        free(a[i].exec_host);
    }
    free(a);
}
