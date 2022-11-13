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
    
    // 2 decode binary obs file, and write ascii file
    decodeBinaryObs(&config);

    // 3 read ascii file to memory
    ObsTable obs_table;
    loadObsTable(&config, &obs_table);

    return 0;
}
