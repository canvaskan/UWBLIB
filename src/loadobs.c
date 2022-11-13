#include "uwblib.h"

void loadObsTable(const Config *config, ObsTable *obs_table)
{
    int obs_i = 0;
    for (int i = 0; i < CONFIG_MAX_OBS_FILE_N; i++)
    {
        char obs_file[MAX_LINE_LEN] = "";
        strcpy(obs_file, config->obs_files[i]);
        FILE *fr = fopen(strcat(obs_file, DECODE_ASCII_OBS_SUFFIX), "r");
        if (!fr)
        {
            printf("loadObsTable cannot open file!\n");
            exit(EXIT_FAILURE);
        }

        char line[MAX_LINE_LEN];
        // skip header
        fgets(line, MAX_LINE_LEN, fr);
        while (fgets(line, MAX_LINE_LEN, fr))
        {
            int rssi, retention, flag;
            char datestr[MAX_LINE_LEN] = "";
            char timestr[MAX_LINE_LEN] = "";
            gtime_t time = {0};

            int n = sscanf(line, "%d %d %d %d %d %d %d %d %d %s %s",
                           &(obs_table->obs_records[obs_i].anchor_id),
                           &(obs_table->obs_records[obs_i].tag_id),
                           &(obs_table->obs_records[obs_i].distance),
                           &rssi,
                           &(obs_table->obs_records[obs_i].battery),
                           &(obs_table->obs_records[obs_i].SOS),
                           &(obs_table->obs_records[obs_i].mobile),
                           &retention,
                           &flag,
                           datestr,
                           timestr);
            if (n != 11)
            {
                printf("WARNING: loadObsTable parse fail at %s!\n", obs_file);
            }

            strcat(datestr, " ");
            strcat(datestr, timestr);
            int res = str2time_json(datestr, 0, 19, &(obs_table->obs_records[obs_i].time));
            if (res < 0) // if time is missing, set time to 0
            {
                memset(&(obs_table->obs_records[obs_i].time), 0, sizeof(gtime_t));
            }

            obs_i++;
        }
    }
    obs_table->obs_n = obs_i;
}

void imputeTime(ObsTable *obs_table)
{
    for (int obs_i = 0; obs_i < obs_table->obs_n; obs_i++)
    {
        // if missing time
        if (obs_table->obs_records[obs_i].time.time == 0)
        {
            // find time before it
            gtime_t time_bef = {0};
            for (int i = 1; i < LOADOBS_MAX_IMPUTE_SPAN; i++)
            {
                if (obs_i - i < 0)
                {
                    break;
                }
                if (obs_table->obs_records[obs_i - i].time.time != 0)
                {
                    time_bef = obs_table->obs_records[obs_i - i].time;
                    break;
                }
            }

            if (time_bef.time == 0)
            {
                printf("WARNING: imputeTime no time before!\n");
            }

            // find time after it
            gtime_t time_aft = {0};
            for (int i = 1; i < LOADOBS_MAX_IMPUTE_SPAN; i++)
            {
                if (obs_i + i > obs_table->obs_n)
                {
                    break;
                }
                if (obs_table->obs_records[obs_i + i].time.time != 0)
                {
                    time_aft = obs_table->obs_records[obs_i + i].time;
                    break;
                }
            }

            if (time_aft.time == 0)
            {
                printf("WARNING: imputeTime no time after!\n");
            }

            // impute time
            if (time_bef.time == 0 && time_aft.time == 0)
            {
                printf("ERROR: imputeTime no time before and after in LOADOBS_MAX_IMPUTE_SPAN, bad data!\n");
                exit(EXIT_FAILURE);
            }
            else if (time_bef.time == 0 && time_aft.time != 0)
            {
                obs_table->obs_records[obs_i].time = time_aft;
            }
            else if (time_bef.time != 0 && time_aft.time == 0)
            {
                obs_table->obs_records[obs_i].time = time_bef;
            }
            else // both time_bef and time_aft have value, use their avrage
            {
                obs_table->obs_records[obs_i].time.time = (time_bef.time + time_aft.time / 2.0);
                obs_table->obs_records[obs_i].time.sec = (time_bef.sec + time_aft.sec / 2.0);
            }
        }
    }
}

static int compareByTime(const void *obs_record1, const void *obs_record2)
{
    double dt = timediff(((ObsRecord *)obs_record1)->time, ((ObsRecord *)obs_record2)->time);
    if (dt == 0.0)
    {
        return 0;
    }
    else
    {
        return dt > 0 ? 1 : -1;
    }
}

void sortByTime(ObsTable *obs_table)
{
    qsort(obs_table->obs_records, obs_table->obs_n, sizeof(ObsRecord), compareByTime);
}

void interpTime(const ObsTable *obs_table, ObsTable *obs_table_new, const gtimeSeries *time_series)
{
    // find unique anchor ids
    int uni_anchor_ids[ANCHOR_MAX_N] = {0};
    for(int obs_i =0;obs_i<obs_table->obs_n;obs_i++)
    {
        int anchor_id = obs_table->obs_records[obs_i].anchor_id;
        uni_anchor_ids[0] = anchor_id;
        // TODO
    }
    // MALLOC!!!
    obs_table_new->obs_n = time_series->n;
    double *x = malloc(obs_table->obs_n * sizeof(double));
    double *y = malloc(obs_table->obs_n * sizeof(double));
    double *x_new = malloc(obs_table_new->obs_n * sizeof(double));
    double *y_new = malloc(obs_table_new->obs_n * sizeof(double));

    // load value, x:time, y:

    // linear interpolation
    // use gsl library


    // FREE!!!
    free(x);
    free(y);
    free(x_new);
    free(y_new);
    x = NULL;
    y = NULL;
    x_new = NULL;
    y_new = NULL;
}