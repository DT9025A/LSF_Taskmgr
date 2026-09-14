#include "views.h"
#include "cluster.h"
#include "table.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void collect_jobs(LineArray *out) {
    char *bo = run_cmd("bjobs 2>&1");
    char *ho = run_cmd("bhosts 2>&1");
    char *lo = run_cmd("lsload 2>&1");

    int n1, n2, n3;
    char **bl = split_lines(bo, &n1);
    char **hl = split_lines(ho, &n2);
    char **ll = split_lines(lo, &n3);

    Table bt = parse_table(bl, n1);
    int nb; BHost  *bh = parse_bhosts(hl, n2, &nb);
    int nu; HostUt *hu = parse_lsload(ll, n3, &nu);

    if (bt.nrows == 0) {
        la_add(out, "No unfinished job found.", C_GRAY);
        goto done;
    }
    {
        int cUser  = table_col(&bt, "USER");
        int cHost  = table_col(&bt, "EXEC_HOST");
        int cQueue = table_col(&bt, "QUEUE");
        int cJob = table_col(&bt, "JOBID");
        const char *user = table_get(&bt, 0, cUser);

        la_addf(out, C_BLUE, "User %s has %d jobs in total.", user, bt.nrows);
        la_add(out, "", C_BLACK);

        for (int i = 0; i < bt.nrows; i++) {
            const char *host = table_get(&bt, i, cHost);
            if (!*host) host = "(unknown)";

            int dup = 0;
            for (int j = 0; j < i; j++) {
                const char *hj = table_get(&bt, j, cHost);
                if (!*hj) hj = "(unknown)";
                if (!strcmp(hj, host)) { dup = 1; break; }
            }
            if (dup) continue;

            int total = 0;
            for (int j = i; j < bt.nrows; j++) {
                const char *hj = table_get(&bt, j, cHost);
                if (!*hj) hj = "(unknown)";
                if (!strcmp(hj, host)) total++;
            }

            StrBuf qs; sb_init(&qs);
            int first_q = 1;
            for (int j = i; j < bt.nrows; j++) {
                const char *hj = table_get(&bt, j, cHost);
                if (!*hj) hj = "(unknown)";
                if (strcmp(hj, host) != 0) continue;
                const char *q = table_get(&bt, j, cQueue);
                if (!*q) continue;

                int seen = 0;
                for (int k = i; k < j; k++) {
                    const char *hk = table_get(&bt, k, cHost);
                    if (!*hk) hk = "(unknown)";
                    if (strcmp(hk, host) != 0) continue;
                    if (!strcmp(table_get(&bt, k, cQueue), q)) { seen = 1; break; }
                }
                if (seen) continue;
                if (!first_q) sb_puts(&qs, ",");
                sb_puts(&qs, q);
                first_q = 0;
            }
            if (!qs.data) sb_puts(&qs, "N/A");

            la_addf(out, C_BLACK, "Host: %s, %s, %d jobs", host, qs.data, total);
            sb_free(&qs);

            const char *st = bh_lookup_status(bh, nb, host);
            if (st) {
                int mx = bh_lookup_max(bh, nb, host);
                int nj = bh_lookup_njobs(bh, nb, host);
                int col = (!strcasecmp(st, "ok")) ? C_GREEN : C_RED;
                la_addf(out, col, "  status: %s (%d of %d used)", st, nj, mx);
            } else {
                la_add(out, "  (bhosts missing)", C_GRAY);
            }

            double ut;
            if (hu_lookup(hu, nu, host, &ut)) {
                int col;
                if      (ut >= 90) col = C_RED;
                else if (ut >= 80) col = C_PURPLE;
                else if (ut >= 60) col = C_BLUE;
                else               col = C_GREEN;

                char ubuf[32];
                if (ut == (int)ut) snprintf(ubuf, sizeof(ubuf), "%.0f%%", ut);
                else               snprintf(ubuf, sizeof(ubuf), "%.1f%%", ut);
                la_addf(out, col, "  utilization: %s", ubuf);
            } else {
                la_add(out, "  (lsload missing)", C_GRAY);
            }

            {
                StrBuf hs; sb_init(&hs); sb_puts(&hs, "  ");
                for (int c = 0; c < bt.nheaders; c++) {
                    if (c) sb_puts(&hs, "  ");
                    sb_puts(&hs, bt.headers[c]);
                }
                la_add(out, hs.data, C_BLUE);
                sb_free(&hs);
            }
            /*for (int j = i; j < bt.nrows; j++) {
                const char *hj = table_get(&bt, j, cHost);
                if (!*hj) hj = "(unknown)";
                if (strcmp(hj, host) != 0) continue;
                StrBuf sb; sb_init(&sb); sb_puts(&sb, "  ");
                for (int c = 0; c < bt.nheaders; c++) {
                    if (c) sb_puts(&sb, "  ");
                    sb_puts(&sb, table_get(&bt, j, c));
                }
                la_add(out, sb.data, C_BLACK);
                sb_free(&sb);
            }*/
            for (int j = i; j < bt.nrows; j++) {
                const char *hj = table_get(&bt, j, cHost);
                if (!*hj) hj = "(unknown)";
                if (strcmp(hj, host) != 0) continue;

                StrBuf sb; sb_init(&sb); sb_puts(&sb, "  ");
                for (int c = 0; c < bt.nheaders; c++) {
                    if (c) sb_puts(&sb, "  ");
                    sb_puts(&sb, table_get(&bt, j, c));
                }
                la_addf_job(out, C_BLACK, table_get(&bt, j, cJob), "%s", sb.data);
                sb_free(&sb);
            }
            la_add(out, "", C_BLACK);
        }
    }
done:
    table_free(&bt);
    free_bhosts(bh, nb);
    free_hus(hu, nu);
    free_strs(bl, n1); free_strs(hl, n2); free_strs(ll, n3);
    free(bo); free(ho); free(lo);
}
