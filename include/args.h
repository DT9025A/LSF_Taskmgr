#ifndef LSF_ARGS_H
#define LSF_ARGS_H

#include <stddef.h>

enum {SHOW_NULL = 0, SHOW_HELP, SHOW_VERSION};
enum {KILL_DISABLE = 0, KILL_ENABLE, KILL_ALL_ENABLE};

typedef struct {
    unsigned char kill_config; //-k[a] ok
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

