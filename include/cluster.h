#ifndef LSF_CLUSTER_H
#define LSF_CLUSTER_H

typedef struct { char *host; char *status; int maxv; int njobs; } BHost;
typedef struct { char *host; double ut; } HostUt;
typedef struct { char *queue; char **hosts; int nhosts, cap; } QueueHosts;

BHost      *parse_bhosts (char **lines, int n, int *out_n);
HostUt     *parse_lsload (char **lines, int n, int *out_n);
QueueHosts *parse_bqueues(char **lines, int n, int *out_n);

void free_bhosts(BHost *a, int n);
void free_hus   (HostUt *a, int n);
void free_qh    (QueueHosts *a, int n);

const char *bh_lookup_status(BHost *bh, int n, const char *host);
int         bh_lookup_max   (BHost *bh, int n, const char *host);
int         bh_lookup_njobs (BHost *bh, int n, const char *host);
int         hu_lookup       (HostUt *hu, int n, const char *host, double *out);

#endif
