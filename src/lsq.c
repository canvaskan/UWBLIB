#include"uwblib.h"

void leastSquareOneEpoch(const ObsTable *obs_table, const gtime_t epoch, const int *unique_tag_ids, const int unique_tag_n, ResTable* res_table)
{
    // for each tag
    for(int tag_i = 0;tag_i<unique_tag_n;tag_i++)
    {
        int tag_id = unique_tag_ids[tag_i];
        leastSquareOneEpochOneTag(obs_table, epoch, tag_id, res_table);
    }
}

void leastSquareOneEpochOneTag(const ObsTable *obs_table, const gtime_t epoch, const int tag_id, ResTable* res_table)
{
    // filter out obs in this epoch
    for(int obs_i=0;obs_i<obs_table->obs_n;obs_i++)
    {
        
    }
}