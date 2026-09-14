#ifndef LSF_RESOURCES_H
#define LSF_RESOURCES_H
#include <time.h>

typedef struct {
    char *job_id;
    char *queue, *from_host, *exec_host;
    int   pids_count;
    double cpu_usage;
    long  elapsed;
} RInfo;

double parse_cpu_time (const char *s);
time_t parse_start_time(const char *s);
int    count_pids     (const char *s);

RInfo *extract_all_rinfos(char **lines, int nlines, int *out_n);
void   free_rinfos(RInfo *a, int n);

#endif
