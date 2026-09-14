#include "args.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AppArgsConf appConfig = {
    KILL_ENABLE, 
    MONO_FONT, 
    CPU_USAGE_THRESHOLD, 
    AUTO_REFRESH_JOBS, 
    AUTO_REFRESH_NODE, 
    AUTO_REFRESH_RSRC
};

int index_args(int argc, char** argv) {
    if (argc < 2)
        return 0;
        
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            printf("Unrecognized argument %s.\n", argv[i]);
            continue;
        }
        
        switch (argv[i][1]) {
            case 'h':
                return SHOW_HELP;
                
            case 'v':
                return SHOW_VERSION;
                
            case 'k':
                appConfig.enable_kill = TRUE;
                printf("Custom config applied: Kill enable = TRUE.\n");
                break;
                
            case 'f':
                appConfig.font = &argv[i][2];
                printf("Custom config applied: Font = %s.\n", appConfig.font);
                break;
                
            case 't':
                appConfig.avg_threshold = atoi(&argv[i][2]);
                printf("Custom config applied: CPU usage threshold = %d.\n", appConfig.avg_threshold);
                break;
                
            case 'i':
                switch (argv[i][2]) {
                    case 'j':
                        appConfig.jobs_referesh_interval = atoi(&argv[i][3]);
                        printf("Custom config applied: Job refresh interval = %d.\n", appConfig.jobs_referesh_interval);
                        break;
                        
                    case 'n':
                        appConfig.node_referesh_interval = atoi(&argv[i][3]);
                        printf("Custom config applied: Nodes refresh interval = %d.\n", appConfig.node_referesh_interval);
                        break;
                        
                    case 'r':
                        appConfig.rsrc_referesh_interval = atoi(&argv[i][3]);
                        printf("Custom config applied: Resource refresh interval = %d.\n", appConfig.rsrc_referesh_interval);
                        break;
                        
                    default:
                        printf("Unrecognized argument %s.\n", argv[i]);
                        break;
                }
                break;
                
            default:
                printf("Unrecognized argument %s.\n", argv[i]);
                break;
        }
    }
    
    return SHOW_NULL;
}
