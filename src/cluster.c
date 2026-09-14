#include "cluster.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

/* ---------- bhosts ---------- */
BHost *parse_bhosts(char **lines, int n, int *out_n) {
    BHost *arr = NULL; int cnt = 0, cap = 0;
    int hdr = -1; char **hs = NULL; int nh = 0;

    for (int i = 0; i < n; i++) {
        if (strstr(lines[i], "HOST_NAME") && strstr(lines[i], "STATUS")) {
            hs = split_ws(lines[i], &nh);
            hdr = i; break;
        }
    }
    if (hdr < 0) { *out_n = 0; return NULL; }

    int cName = -1, cStat = -1, cMax = -1, cNJobs = -1;
    for (int i = 0; i < nh; i++) {
        if      (!strcmp(hs[i], "HOST_NAME")) cName  = i;
        else if (!strcmp(hs[i], "STATUS"))    cStat  = i;
        else if (!strcmp(hs[i], "MAX"))       cMax   = i;
        else if (!strcmp(hs[i], "NJOBS"))     cNJobs = i;
    }
    for (int i = hdr + 1; i < n; i++) {
        int nf; char **f = split_ws(lines[i], &nf);
        if (nf < nh) { free_strs(f, nf); continue; }
        if (cnt >= cap) { cap = cap ? cap * 2 : 32;
            arr = (BHost *)realloc(arr, cap * sizeof(BHost)); }
        BHost b; memset(&b, 0, sizeof(b));
        b.host   = xstrdup(f[cName]);
        b.status = cStat >= 0 ? xstrdup(f[cStat]) : xstrdup("");
        b.maxv   = cMax   >= 0 ? atoi(f[cMax])    : 0;
        b.njobs  = cNJobs >= 0 ? atoi(f[cNJobs])  : 0;
        arr[cnt++] = b;
        free_strs(f, nf);
    }
    free_strs(hs, nh);
    *out_n = cnt;
    return arr;
}

/* ---------- lsload ---------- */
HostUt *parse_lsload(char **lines, int n, int *out_n) {
    HostUt *arr = NULL; int cnt = 0, cap = 0;
    int hdr = -1; char **hs = NULL; int nh = 0;
    for (int i = 0; i < n; i++) {
        if (strstr(lines[i], "HOST_NAME") && strstr(lines[i], "ut")) {
            hs = split_ws(lines[i], &nh);
            hdr = i; break;
        }
    }
    if (hdr < 0) { *out_n = 0; return NULL; }

    int cName = -1, cUt = -1;
    for (int i = 0; i < nh; i++) {
        if      (!strcmp(hs[i], "HOST_NAME")) cName = i;
        else if (!strcmp(hs[i], "ut"))        cUt   = i;
    }
    for (int i = hdr + 1; i < n; i++) {
        int nf; char **f = split_ws(lines[i], &nf);
        if (nf < nh) { free_strs(f, nf); continue; }
        if (cnt >= cap) { cap = cap ? cap * 2 : 32;
            arr = (HostUt *)realloc(arr, cap * sizeof(HostUt)); }
        arr[cnt].host = xstrdup(f[cName]);
        char *u = f[cUt]; while (*u == ' ') u++;
        char *pct = strchr(u, '%'); if (pct) *pct = 0;
        arr[cnt].ut = atof(u);
        cnt++;
        free_strs(f, nf);
    }
    free_strs(hs, nh);
    *out_n = cnt;
    return arr;
}

/* ---------- bqueues -l ---------- */
QueueHosts *parse_bqueues(char **lines, int n, int *out_n) {
    QueueHosts *arr = NULL; int cnt = 0, cap = 0;
    int cur = -1;
    for (int i = 0; i < n; i++) {
        const char *p = lines[i];
        while (*p == ' ' || *p == '\t') p++;

        if (strncmp(p, "QUEUE:", 6) == 0) {
            char *sp = (char *)p + 6;
            while (*sp == ' ' || *sp == '\t') sp++;
            char *e = sp; while (*e && *e != ' ' && *e != '\t') e++;
            if (cnt >= cap) { cap = cap ? cap * 2 : 8;
                arr = (QueueHosts *)realloc(arr, cap * sizeof(QueueHosts)); }
            arr[cnt].queue  = xstrndup(sp, (size_t)(e - sp));
            arr[cnt].hosts  = NULL;
            arr[cnt].nhosts = 0;
            arr[cnt].cap    = 0;
            cur = cnt++;
        } else if (strncmp(p, "HOSTS:", 6) == 0 && cur >= 0) {
            char *sp = (char *)p + 6;
            while (*sp == ' ' || *sp == '\t') sp++;
            int nh; char **hh = split_ws(sp, &nh);
            for (int k = 0; k < nh; k++) {
                if (arr[cur].nhosts >= arr[cur].cap) {
                    arr[cur].cap = arr[cur].cap ? arr[cur].cap * 2 : 16;
                    arr[cur].hosts = (char **)realloc(arr[cur].hosts,
                                                      arr[cur].cap * sizeof(char *));
                }
                arr[cur].hosts[arr[cur].nhosts++] = hh[k];
            }
            free(hh);
        }
    }
    *out_n = cnt;
    return arr;
}

/* ---------- free ---------- */
void free_bhosts(BHost *a, int n) {
    if (!a) return;
    for (int i = 0; i < n; i++) { free(a[i].host); free(a[i].status); }
    free(a);
}
void free_hus(HostUt *a, int n) {
    if (!a) return;
    for (int i = 0; i < n; i++) free(a[i].host);
    free(a);
}
void free_qh(QueueHosts *a, int n) {
    if (!a) return;
    for (int i = 0; i < n; i++) {
        free(a[i].queue);
        for (int j = 0; j < a[i].nhosts; j++) free(a[i].hosts[j]);
        free(a[i].hosts);
    }
    free(a);
}

/* ---------- lookups ---------- */
const char *bh_lookup_status(BHost *bh, int n, const char *host) {
    for (int i = 0; i < n; i++)
        if (!strcmp(bh[i].host, host)) return bh[i].status;
    return NULL;
}
int bh_lookup_max(BHost *bh, int n, const char *host) {
    for (int i = 0; i < n; i++)
        if (!strcmp(bh[i].host, host)) return bh[i].maxv;
    return 0;
}
int bh_lookup_njobs(BHost *bh, int n, const char *host) {
    for (int i = 0; i < n; i++)
        if (!strcmp(bh[i].host, host)) return bh[i].njobs;
    return 0;
}
int hu_lookup(HostUt *hu, int n, const char *host, double *out) {
    for (int i = 0; i < n; i++)
        if (!strcmp(hu[i].host, host)) { *out = hu[i].ut; return 1; }
    return 0;
}
