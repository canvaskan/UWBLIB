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

    // 1 PREPROCESS OBS
    // 1.1 decode binary obs file, and write ascii file
    decodeBinaryObs(&config);

    // 1.2 read ascii file to memory
    ObsTable obs_table = {0};
    loadObsTable(&config, &obs_table);

    // 1.3 impute obs record that miss time
    imputeTime(&obs_table);

    // 1.4 sort all obs
    sortByTime(&obs_table);

    // 1.5 get unique obs anchor ids and tag ids
    int unique_anchor_ids[ANCHOR_MAX_ID_N] = {0};
    int unique_tag_ids[LOADOBS_MAX_TAG_N] = {0};
    int unique_anchor_n = uniqueAnchorID(&obs_table, unique_anchor_ids);
    int unique_tag_n = uniqueTagID(&obs_table, unique_tag_ids);

    // 1.6 create time series MALLOC!!!
    gtimeSeries time_series = {0};
    makeTimeSeries(&time_series, config.start_time, config.end_time, config.interval_sec);

    // 1.7 interp obs to new obs at time series
    ObsTable obs_table_new = {0};
    interpObsTable(&obs_table, &obs_table_new, &time_series, unique_anchor_ids, unique_anchor_n, unique_tag_ids, unique_tag_n);
    freeObsTable(&obs_table);

    // 1.8 sort all obs again
    sortByTime(&obs_table_new);

    // 1.9 get remained unique obs anchor ids and tag ids
    memset(unique_anchor_ids, 0, ANCHOR_MAX_ID_N * sizeof(int));
    memset(unique_tag_ids, 0, LOADOBS_MAX_TAG_N * sizeof(int));
    unique_anchor_n = uniqueAnchorID(&obs_table_new, unique_anchor_ids);
    unique_tag_n = uniqueTagID(&obs_table_new, unique_tag_ids);

    // 2 ANCHOR INFO
    // 2.1 read anchor file
    AnchorTable anchor_table = {0};
    readAnchorTable(&config, &anchor_table);

    // 3 PROCESS BY EPOCH or BY TAG???
    ResTable res_table = {0};
    res_table.n=time_series.n;
    res_table.res_records = malloc(sizeof(ResRecord)*res_table.n);
    for(int epoch_i = 0; epoch_i<time_series.n; epoch_i++)
    {
        leastSquareOneEpoch(&anchor_table, &obs_table_new, time_series.times[epoch_i], unique_tag_ids, unique_tag_n, &res_table);
    }

    // Free!!!
    freeTimeSeries(&time_series);
    freeObsTable(&obs_table_new);



    return 0;
}
