#include "views.h"
#include "cluster.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct { int free_slots; double ut; const char *host; } Node;

static int cmp_ut_free(const void *A, const void *B) {
    const Node *x = (const Node *)A, *y = (const Node *)B;
    if (x->ut < y->ut) return -1;
    if (x->ut > y->ut) return  1;
    return y->free_slots - x->free_slots;
}
static int cmp_host(const void *A, const void *B) {
    return strcmp(((const Node *)A)->host, ((const Node *)B)->host);
}

void collect_nodes(LineArray *out) {
    char *qo = run_cmd("bqueues -l 2>&1");
    char *ho = run_cmd("bhosts 2>&1");
    char *lo = run_cmd("lsload 2>&1");

    int nq, nh, nl;
    char **ql = split_lines(qo, &nq);
    char **hl = split_lines(ho, &nh);
    char **ll = split_lines(lo, &nl);

    int nqh; QueueHosts *qh = parse_bqueues(ql, nq, &nqh);
    int nb;  BHost      *bh = parse_bhosts (hl, nh, &nb);
    int nu;  HostUt     *hu = parse_lsload (ll, nl, &nu);

    const char *order[] = { "h_queue", "m_queue", "l_queue" };
    int norder = 3, any = 0;

    for (int oi = 0; oi < norder; oi++) {
        for (int qi = 0; qi < nqh; qi++) {
            if (strcmp(qh[qi].queue, order[oi]) != 0) continue;
            any = 1;

            int cnt = qh[qi].nhosts;
            Node *nodes = (Node *)calloc((size_t)(cnt ? cnt : 1), sizeof(Node));
            for (int k = 0; k < cnt; k++) {
                const char *h = qh[qi].hosts[k];
                int mx = bh_lookup_max(bh, nb, h);
                int nj = bh_lookup_njobs(bh, nb, h);
                int fr = mx - nj; if (fr < 0) fr = 0;
                double ut = 0.0; hu_lookup(hu, nu, h, &ut);
                nodes[k].free_slots = fr;
                nodes[k].ut         = ut;
                nodes[k].host       = h;
            }

            Node *g10 = (Node *)calloc((size_t)(cnt ? cnt : 1), sizeof(Node));
            Node *g19 = (Node *)calloc((size_t)(cnt ? cnt : 1), sizeof(Node));
            Node *g0  = (Node *)calloc((size_t)(cnt ? cnt : 1), sizeof(Node));
            int n10 = 0, n19 = 0, n0 = 0;
            for (int k = 0; k < cnt; k++) {
                if      (nodes[k].free_slots >= 10) g10[n10++] = nodes[k];
                else if (nodes[k].free_slots  >  0) g19[n19++] = nodes[k];
                else                                g0 [n0 ++] = nodes[k];
            }
            qsort(g10, (size_t)n10, sizeof(Node), cmp_ut_free);
            qsort(g19, (size_t)n19, sizeof(Node), cmp_ut_free);
            qsort(g0,  (size_t)n0,  sizeof(Node), cmp_host);

            la_addf(out, C_BLUE, "=== Queue: %s (%d nodes) ===", qh[qi].queue, cnt);

            Node *all[3]  = { g10, g19, g0 };
            int   alln[3] = { n10, n19, n0 };
            for (int g = 0; g < 3; g++) {
                for (int k = 0; k < alln[g]; k++) {
                    const char *h = all[g][k].host;
                    const char *prefix = (g == 0) ? "*** " : (g == 1 ? "  * " : "    ");
                    int col = (g == 0) ? C_BLUE : (g == 1 ? C_BLACK : C_GRAY);
                    la_addf(out, col, "%s%s [free: %d]", prefix, h, all[g][k].free_slots);

                    const char *st = bh_lookup_status(bh, nb, h);
                    /*char status_buf[128];
                    int  col2 = C_GRAY;
                    if (st) {
                        int mx = bh_lookup_max(bh, nb, h);
                        int nj = bh_lookup_njobs(bh, nb, h);
                        snprintf(status_buf, sizeof(status_buf),
                                 " %s (%d of %d used)", st, nj, mx);
                        col2 = !strcasecmp(st, "ok") ? C_GREEN : C_RED;
                    } else {
                        snprintf(status_buf, sizeof(status_buf), " (bhosts missing)");
                    }
                    // status line - keep bhosts status color 
                    la_addf(out, col2, "       %s", status_buf);

                    // ut line - colored by the new utilization rule 
                    double ut = 0.0;
                    char ut_buf[64];
                    int  ut_col = C_GRAY;
                    if (hu_lookup(hu, nu, h, &ut)) {
                        if      (ut >= 90) ut_col = C_RED;
                        else if (ut >= 80) ut_col = C_PURPLE;
                        else if (ut >= 60) ut_col = C_BLUE;
                        else               ut_col = C_GREEN;

                        if (ut == (int)ut) snprintf(ut_buf, sizeof(ut_buf), "ut: %.0f%%", ut);
                        else               snprintf(ut_buf, sizeof(ut_buf), "ut: %.1f%%", ut);
                    } else {
                        snprintf(ut_buf, sizeof(ut_buf), "ut: N/A");
                    }
                    la_addf(out, ut_col, "       %s", ut_buf);*/
                    
                    /* Build status string (same content as before). */
                    char status_buf[128];
                    int  status_col = C_GRAY;
                    if (st) {
                        int mx = bh_lookup_max(bh, nb, h);
                        int nj = bh_lookup_njobs(bh, nb, h);
                        snprintf(status_buf, sizeof(status_buf),
                                 " %s (%d of %d used)", st, nj, mx);
                        status_col = !strcasecmp(st, "ok") ? C_GREEN : C_RED;
                    } else {
                        snprintf(status_buf, sizeof(status_buf), " (bhosts missing)");
                    }

                    /* Build ut string with the new color rule. */
                    char ut_buf[64];
                    int  ut_col = C_GRAY;
                    double ut = 0.0;
                    if (hu_lookup(hu, nu, h, &ut)) {
                        if      (ut >= 90) ut_col = C_RED;
                        else if (ut >= 80) ut_col = C_PURPLE;
                        else if (ut >= 60) ut_col = C_BLUE;
                        else               ut_col = C_GREEN;

                        if (ut == (int)ut) snprintf(ut_buf, sizeof(ut_buf), "  ut: %.0f%%", ut);
                        else               snprintf(ut_buf, sizeof(ut_buf), "  ut: %.1f%%", ut);
                    } else {
                        snprintf(ut_buf, sizeof(ut_buf), "  ut: N/A");
                    }

                    /* One line, multiple colors. */
                    SegSpec segs[3] = {
                        { "       ",  C_BLACK  },   /* 7-space indent, same as before */
                        { status_buf, status_col },
                        { ut_buf,     ut_col     },
                    };
                    la_add_segs(out, segs, 3, NULL);
                }
            }
            la_add(out, "", C_BLACK);
            free(g10); free(g19); free(g0); free(nodes);
        }
    }
    if (!any) la_add(out, "No queue information found.", C_GRAY);

    free_qh(qh, nqh);
    free_bhosts(bh, nb);
    free_hus(hu, nu);
    free_strs(ql, nq); free_strs(hl, nh); free_strs(ll, nl);
    free(qo); free(ho); free(lo);
}
