#include "uwblib.h"

static const char USAGE[] = "USAGE: UWBLIB config_file\n";

int main(int argc, char const *argv[])
{
    if(argc != 2){
        printf("%s", USAGE);
        exit(EXIT_FAILURE);
    }

    // 0 read config
    Config config = {0};
    readConfig(argv[1], &config);

    // 1 read anchor file
    AnchorTable anchor_table = {0};
    readAnchorTable(&config, &anchor_table);
    
    // 2 decode binary obs file, and write ascii file
    decodeBinaryObs(&config);

    // 3 read ascii file to memory
    ObsTable obs_table = {0};
    loadObsTable(&config, &obs_table);

    // 4 impute obs record that miss time
    imputeTime(&obs_table);

    // 5 sort all obs
    sortByTime(&obs_table);

    // 6 get unique obs anchor ids and tag ids
    int unique_anchor_ids[ANCHOR_MAX_ID_N] = {0};
    int unique_tag_ids[LOADOBS_MAX_TAG_N] = {0};
    int unique_anchor_n = uniqueAnchorID(&obs_table, unique_anchor_ids);
    int unique_tag_n = uniqueTagID(&obs_table, unique_tag_ids);

    // 7 create time series MALLOC!!!
    gtimeSeries time_series = {0};
    makeTimeSeries(&time_series, config.start_time, config.end_time, config.interval_sec);

    // 8 interp obs to new obs at time series
    ObsTable obs_table_new = {0};
    interpObsTable(&obs_table, &obs_table_new, &time_series, unique_anchor_ids, unique_anchor_n, unique_tag_ids, unique_tag_n);



    // Free!!!
    freeTimeSeries(&time_series);



    return 0;
}
