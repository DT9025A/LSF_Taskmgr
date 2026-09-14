#include "app.h"
#include "args.h"
#include "config.h"
#include <stdio.h>

int main(int argc, char** argv) {
                
    switch (index_args(argc, argv)) {
        case SHOW_HELP:
            printf ("\n\
    %s help list: \n\n\
    -h\n\
        Show this message, usage: lsf_taskmgr -h\n\
    -v\n\
        Show app version, usage: lsf_taskmgr -v\n\
    -k (default %s)\n\
        Enable kill job, usage: lsf_taskmgr -k\n\
    -f (default %s)\n\
        Use user-defined X11 font, usage: lsf_taskmgr -flucidasans-bold-24\n\
    -t (default %d)\n\
        Define average CPU usage highlight threshold (low), usage: lsf_taskmgr -t50\n\
    -ij (default %d)\n\
        Define refresh interval of jobs tab (secs), usage: lsf_taskmgr -ij5\n\
    -in (default %d)\n\
        Define refresh interval of nodes tab (secs), usage: lsf_taskmgr -in5\n\
    -ir (default %d)\n\
        Define refresh interval of resources tab (secs), usage: lsf_taskmgr -ir5\n\
    ", WINDOW_TITLE, KILL_ENABLE ? "TRUE" : "FALSE", MONO_FONT, CPU_USAGE_THRESHOLD, AUTO_REFRESH_JOBS, AUTO_REFRESH_NODE, AUTO_REFRESH_RSRC);
        
        case SHOW_VERSION:
            printf ("\n    %s, version %d.%d, DT9025A, build at %s %s\n\n", 
                WINDOW_TITLE, APPLICATION_VERSION_MAJOR, APPLICATION_VERSION_MINOR, __DATE__, __TIME__);
        return 0;
    }
    
    return app_run();
}
