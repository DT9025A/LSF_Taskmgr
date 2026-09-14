#include "views.h"
#include "jobinfo.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int next_token(const char *s, int *pos, char *out, int outsz) {
    int i = *pos;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (!s[i]) { *pos = i; return 0; }
    int j = 0;
    while (s[i] && s[i] != ' ' && s[i] != '\t') {
        if (j < outsz - 1) out[j++] = s[i];
        i++;
    }
    out[j] = 0;
    *pos = i;
    return 1;
}

void collect_info(LineArray *out, const char *info_ids) {
    if (!info_ids || !*info_ids) {
        la_add(out, "Enter LSF job ID(s) and press Enter to query.", C_GRAY);
        return;
    }
    int pos = 0; char jid[64];
    while (next_token(info_ids, &pos, jid, sizeof(jid))) {
        la_addf(out, C_BLUE, "Job ID: %s", jid);

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "bjobs -W %s 2>&1", jid);
        char *raw = run_cmd(cmd);
        char *script = extract_script_path(raw);
        if (!script) {
            la_add(out, "  No matching run script found.", C_RED);
            la_add(out, "", C_BLACK);
            free(raw);
            continue;
        }
        la_addf(out, C_BLACK, "Run Script: %s", script);

        struct stat st;
        if (stat(script, &st) != 0) {
            la_add(out, "  Script does not exist or not readable.", C_RED);
            la_add(out, "", C_BLACK);
            free(script); free(raw);
            continue;
        }
        char *content = read_file(script);
        if (!content) {
            la_add(out, "  Cannot read script.", C_RED);
            la_add(out, "", C_BLACK);
            free(script); free(raw);
            continue;
        }
        int stype = -1;
        if      (strstr(script, "runICRP"))      stype = 0;
        else if (strstr(script, "swiftJob"))     stype = 1;
        else if (strstr(script, "runGenricJob")) stype = 2;

        if (stype < 0) {
            la_add(out, "  Unknown script type.", C_RED);
            free(content); free(script); free(raw);
            la_add(out, "", C_BLACK);
            continue;
        }
        const char *tn[] = { "runICRP", "swiftJob", "runGenricJob" };
        la_addf(out, C_BLACK, "Script Type: %s", tn[stype]);

        char *cd = get_cd_path(content);
        la_addf(out, C_BLACK, "CD Path: %s", cd ? cd : "N/A");
        free(cd);

        if (stype == 0) {
            char *mp, *lf, *mh;
            extract_runICRP_params(content, &mp, &lf, &mh);
            la_addf(out, C_BLACK, "mpssession: %s", mp ? mp : "N/A");
            la_addf(out, C_BLACK, "log: %s",        lf ? lf : "N/A");
            la_addf(out, C_BLACK, "mpshost: %s",    mh ? mh : "N/A");
            if (lf) {
                char *lib, *cell, *state;
                extract_lib_cell_state(lf, &lib, &cell, &state);
                la_addf(out, C_BLACK, "Library: %s",    lib   ? lib   : "N/A");
                la_addf(out, C_BLACK, "Cell: %s",       cell  ? cell  : "N/A");
                la_addf(out, C_BLACK, "State Name: %s", state ? state : "N/A");
                free(lib); free(cell); free(state);
            }
            free(mp); free(lf); free(mh);
        } else if (stype == 2) {
            GenericInfo gi;
            if (extract_runGenricJob_info(script, &gi)) {
                la_addf(out, C_BLACK, "Library: %s",     gi.library);
                la_addf(out, C_BLACK, "Cell: %s",        gi.cell);
                la_addf(out, C_BLACK, "SimView: %s",     gi.simview);
                la_addf(out, C_BLACK, "HistoryView: %s", gi.history_view);
                la_addf(out, C_BLACK, "TestName: %s",    gi.testname);
                free_generic(&gi);
            } else {
                la_add(out, "  Failed to parse runGenricJob path.", C_RED);
            }
        } else {
            char *hn, *bh, *sl, *sc, *sv;
            extract_swiftJob_params(content, &hn, &bh, &sl, &sc, &sv);
            la_addf(out, C_BLACK, "historyName: %s", hn ? hn : "N/A");
            la_addf(out, C_BLACK, "beanhost: %s",    bh ? bh : "N/A");
            la_addf(out, C_BLACK, "sdbLib: %s",      sl ? sl : "N/A");
            la_addf(out, C_BLACK, "sdbCell: %s",     sc ? sc : "N/A");
            la_addf(out, C_BLACK, "sdbView: %s",     sv ? sv : "N/A");
            free(hn); free(bh); free(sl); free(sc); free(sv);

            char *jid2 = extract_job_id_from_script_path(script);
            if (jid2) {
                char *lp = find_and_resolve_log_path(content, jid2);
                if (lp) {
                    la_addf(out, C_BLACK, "SimJobLog: %s", lp);
                    char *h, *p, *t, *c;
                    if (parse_last_json_record(lp, &h, &p, &t, &c)) {
                        la_addf(out, C_BLACK, "  history: %s",    h);
                        la_addf(out, C_BLACK, "  point: %s",      p);
                        la_addf(out, C_BLACK, "  testname: %s",   t);
                        la_addf(out, C_BLACK, "  cornername: %s", c);
                        free(h); free(p); free(t); free(c);
                    } else {
                        la_add(out, "  No JSON record found.", C_GRAY);
                    }
                    free(lp);
                } else {
                    la_add(out, "SimLog: N/A (CDS_SIM_MONITOR_LOG_FILE not found)", C_GRAY);
                }
                free(jid2);
            }
        }
        free(content); free(script); free(raw);
        la_add(out, "", C_BLACK);
    }
}
