#ifndef LSF_JOBINFO_H
#define LSF_JOBINFO_H

char *strip_quotes(const char *v);
char *extract_script_path(const char *bjobs_out);
char *get_cd_path(const char *script);

void extract_runICRP_params(const char *content,
                            char **mpssession, char **logf, char **mpshost);
void extract_swiftJob_params(const char *content,
                             char **hn, char **bh, char **sl,
                             char **sc, char **sv);
void extract_lib_cell_state(const char *log_path,
                            char **lib, char **cell, char **state);

char *extract_job_id_from_script_path(const char *p);
char *find_and_resolve_log_path(const char *content, const char *job_id);
char *json_get(const char *line, const char *key);
int   parse_last_json_record(const char *log_path,
                             char **history, char **point,
                             char **testname, char **cornername);

typedef struct {
    char *library, *cell, *simview, *history_view, *testname;
} GenericInfo;

int  extract_runGenricJob_info(const char *path, GenericInfo *out);
void free_generic(GenericInfo *g);

#endif
