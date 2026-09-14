#ifndef LSF_ARGS_H
#define LSF_ARGS_H

#include <stddef.h>

enum {SHOW_NULL = 0, SHOW_HELP, SHOW_VERSION};

typedef struct {
    unsigned char enable_kill; //-k ok
    char *font; //-f ok
    int  avg_threshold; //-t ok
    int  jobs_referesh_interval; //-ij
    int  node_referesh_interval; //-in
    int  rsrc_referesh_interval; //-ir
} AppArgsConf;

extern AppArgsConf appConfig;

// return 1: show help
// return 2: show version
int index_args(int argc, char** argv);

#endif

