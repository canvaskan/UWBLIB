#include "uwblib.h"

static const char USAGE[] = "USAGE: UWBLIB config_file\n";

int main(int argc, char const *argv[])
{
    if(argc != 2){
        printf("%s", USAGE);
        exit(EXIT_FAILURE);
    }

    // 0 read config
    Config config;
    readConfig(argv[1], &config);

    // 1 read anchor file
    AnchorTable anchor_table;
    readAnchorTable(&config, &anchor_table);
    
    // 2 read and decode binary obs file
    decodeBinaryObs(&config);

    ObsTable obs_table;
    HeartbeatTable heartbeat_table;

    return 0;
}
