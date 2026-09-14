#include "views.h"
#include "resources.h"
#include "util.h"
#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void collect_rsrc(LineArray *out) {
    char *raw = run_cmd("bjobs -W 2>&1");
    int n; char **lines = split_lines(raw, &n);

    int nr = 0;
    RInfo *arr = extract_all_rinfos(lines, n, &nr);

    if (nr == 0) {
        la_add(out, "No running jobs.", C_GRAY);
    } else {
        char header[256];
        snprintf(header, sizeof(header),
                 "%-8s %-10s %-22s %-14s %-8s %-10s %-10s",
                 "JobID", "Queue", "Exec Host",
                 "Run time", "Threads", "Avg CPU", "CPU/Thr");
        la_add(out, header, C_BLUE);
        {
            int hl = (int)strlen(header);
            char *sep = (char *)malloc((size_t)hl + 1);
            memset(sep, '-', (size_t)hl); sep[hl] = 0;
            la_add(out, sep, C_BLUE);
            free(sep);
        }
        for (int i = 0; i < nr; i++) {
            long eh = arr[i].elapsed;
            char rt[32];
            snprintf(rt, sizeof(rt), "%ld:%02ld:%02ld",
                     eh / 3600, (eh % 3600) / 60, eh % 60);

            double cpu_thr = arr[i].pids_count > 0
                           ? arr[i].cpu_usage / arr[i].pids_count : 0.0;
            int col = cpu_thr < appConfig.avg_threshold ? C_RED : C_BLACK;

            char cpu_s[16], thr_s[16], q_s[24], e_s[40];
            snprintf(cpu_s, sizeof(cpu_s), "%.2f%%", arr[i].cpu_usage);
            snprintf(thr_s, sizeof(thr_s), "%.2f%%", cpu_thr);
            snprintf(q_s,   sizeof(q_s),   "%.*s", 10,
                     arr[i].queue     ? arr[i].queue     : "");
            snprintf(e_s,   sizeof(e_s),   "%.*s", 22,
                     arr[i].exec_host ? arr[i].exec_host : "");

            la_addf_job(out, col, arr[i].job_id,
                        "%-8s %-10s %-22s %-14s %-8d %-10s %-10s",
                        arr[i].job_id, q_s, e_s, rt, arr[i].pids_count, cpu_s, thr_s);
        }
    }
    free_rinfos(arr, nr);
    free_strs(lines, n);
    free(raw);
}

