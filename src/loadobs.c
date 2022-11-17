#include "uwblib.h"

void loadObsTable(const Config *config, ObsTable *obs_table)
{
    int obs_i = 0;

    // count how many records are there
    for (int i = 0; i < CONFIG_MAX_OBS_FILE_N; i++)
    {
        if (config->obs_files[i][0] == '\0')
        {
            continue;
        }
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
            obs_i++;
        }
    }

    // allocate memory for ObsTable
    obs_table->obs_n = obs_i;
    obs_table->obs_records = malloc(obs_i * sizeof(ObsRecord));
    if (obs_table->obs_records == NULL)
    {
        printf("ERROR: loadObsTable cannot allocate memory!\n");
        exit(EXIT_FAILURE);
    }

    // load the obs records
    obs_i = 0;
    for (int i = 0; i < CONFIG_MAX_OBS_FILE_N; i++)
    {
        if (config->obs_files[i][0] == '\0')
        {
            continue;
        }
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
        fclose(fr);
    }
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

int uniqueAnchorID(const ObsTable *obs_table, int *unique_anchor_ids)
{
    // find unique anchor ids
    int uni_anchor_i = 0;
    for (int obs_i = 0; obs_i < obs_table->obs_n; obs_i++)
    {
        int is_unique = 1;
        int anchor_id = obs_table->obs_records[obs_i].anchor_id;
        for (int i = 0; i < uni_anchor_i; i++)
        {
            if (anchor_id == unique_anchor_ids[i])
            {
                is_unique = 0;
                break;
            }
        }
        if (is_unique)
        {
            unique_anchor_ids[uni_anchor_i] = anchor_id;
            uni_anchor_i++;
        }
    }
    if (uni_anchor_i > ANCHOR_MAX_ID_N)
    {
        printf("ERROR: uniqueAnchorID detect %d Anchor ID > ANCHOR_MAX_ID_N = %d, need to change ANCHOR_MAX_ID_N value!\n",
               uni_anchor_i, ANCHOR_MAX_ID_N);
        exit(EXIT_FAILURE);
    }
    return uni_anchor_i;
}

int uniqueTagID(const ObsTable *obs_table, int *unique_tag_ids)
{
    // find unique tag ids
    int uni_tag_i = 0;
    for (int obs_i = 0; obs_i < obs_table->obs_n; obs_i++)
    {
        int is_unique = 1;
        int tag_id = obs_table->obs_records[obs_i].tag_id;
        for (int i = 0; i < uni_tag_i; i++)
        {
            if (tag_id == unique_tag_ids[i])
            {
                is_unique = 0;
                break;
            }
        }
        if (is_unique)
        {
            unique_tag_ids[uni_tag_i] = tag_id;
            uni_tag_i++;
        }
    }
    if (uni_tag_i > LOADOBS_MAX_TAG_N)
    {
        printf("ERROR: uniqueTagID detect %d Anchor ID > LOADOBS_MAX_TAG_N = %d, need to change LOADOBS_MAX_TAG_N value!\n",
               uni_tag_i, LOADOBS_MAX_TAG_N);
        exit(EXIT_FAILURE);
    }
    return uni_tag_i;
}

void interpObsTable(const ObsTable *obs_table, ObsTable *obs_table_new, const gtimeSeries *time_series,
                    const int *unique_anchor_ids, const int unique_anchor_n, const int *unique_tag_ids, const int unique_tag_n)
{
    int obs_new_i = 0;
    int reserve_n = time_series->n * unique_anchor_n * unique_tag_n;
    obs_table_new->obs_records = malloc(reserve_n * sizeof(ObsRecord));
    if (obs_table_new->obs_records == NULL)
    {
        printf("ERROR: interpObsTable cannot allocate memory!\n");
        exit(EXIT_FAILURE);
    }

    // new time sample points
    double *x_new = malloc(time_series->n * sizeof(double));
    for (int i = 0; i < time_series->n; i++)
    {
        x_new[i] = time_series->times[i].time + time_series->times[i].sec;
    }

    // for each anchor-tag pair, do linear interp
    for (int anchor_i = 0; anchor_i < unique_anchor_n; anchor_i++)
    {
        int anchor_id = unique_anchor_ids[anchor_i];
        if (anchor_id == 0)
        {
            continue;
        }
        for (int tag_i = 0; tag_i < unique_tag_n; tag_i++)
        {
            int tag_id = unique_tag_ids[tag_i];
            if (tag_id == 0)
            {
                continue;
            }
            printf("INFO: processing pair anchor_id=%d, tag_id=%d\n", anchor_id, tag_id);

            // count how many data record this anchor-tag pair has
            int data_i = 0;
            for (int obs_i = 0; obs_i < obs_table->obs_n; obs_i++)
            {
                // if match anchor and tag id
                if (obs_table->obs_records[obs_i].anchor_id == anchor_id && obs_table->obs_records[obs_i].tag_id == tag_id)
                {
                    data_i++;
                }
            }
            printf("INFO: pair anchor_id=%d, tag_id=%d has %d data records\n", anchor_id, tag_id, data_i);

            // allocate fitting array
            double *x = malloc(data_i * sizeof(double));
            double *y = malloc(data_i * sizeof(double));
            double *y_new = malloc(time_series->n * sizeof(double));

            // load data of anchor-tag pair
            int xi = 0;
            for (int obs_i = 0; obs_i < obs_table->obs_n; obs_i++)
            {
                // if match anchor and tag id
                if (obs_table->obs_records[obs_i].anchor_id == anchor_id && obs_table->obs_records[obs_i].tag_id == tag_id)
                {
                    x[xi] = obs_table->obs_records[obs_i].time.time + obs_table->obs_records[obs_i].time.sec;
                    y[xi] = obs_table->obs_records[obs_i].distance;
                    // if same time measurement, add a little time to keep x incremental
                    if (xi > 1 && x[xi] - x[xi - 1] < 0.1)
                    {
                        x[xi] += 0.1;
                    }
                    xi++;
                }
            }

            // check x_new is in range(x.min, x.max)
            int is_in_range = 1;
            double xmin = gsl_stats_min(x, 1, data_i);
            double xmax = gsl_stats_max(x, 1, data_i);
            for (int i = 0; i < time_series->n; i++)
            {
                if (x_new[i] > xmax)
                {
                    printf("WARNING: interpObsTable anchor_id=%d, tag_id=%d, time x_new[i]=%lf>xmax=%lf!\n", anchor_id, tag_id, x_new[i], xmax);
                    is_in_range = 0;
                    break;
                }
                else if (x_new[i] < xmin)
                {
                    printf("WARNING: interpObsTable anchor_id=%d, tag_id=%d, time x_new[i]=%lf<xmin=%lf!\n", anchor_id, tag_id, x_new[i], xmin);
                    is_in_range = 0;
                    break;
                }
            }
            if (is_in_range == 0)
            {
                printf("WARNING: skip processing pair anchor_id=%d, tag_id=%d\n", anchor_id, tag_id);
                free(x);
                free(y);
                free(y_new);
                continue;
            }

            // linear interp
            linearInterp(x, y, data_i, x_new, y_new, time_series->n);

            // store data in obs table new
            obs_table_new->obs_n += time_series->n;
            for (int i = 0; i < time_series->n; i++)
            {
                int is_interp = 1;
                for (int j = 0; j < data_i; j++)
                {
                    // if interp point is very near to the actual obs point, it is considered to be actual obs. //TODO make them more weights
                    if (fabs(x_new[i] - x[j]) < 0.1)
                    {
                        is_interp = 0;
                        break;
                    }
                }
                obs_table_new->obs_records[obs_new_i].time.time = floor(x_new[i]);
                obs_table_new->obs_records[obs_new_i].time.sec = x_new[i] - floor(x_new[i]);
                obs_table_new->obs_records[obs_new_i].distance = y_new[i];
                obs_table_new->obs_records[obs_new_i].anchor_id = anchor_id;
                obs_table_new->obs_records[obs_new_i].tag_id = tag_id;
                obs_table_new->obs_records[obs_new_i].is_interp = is_interp;
                obs_new_i++;
            }

            free(x);
            free(y);
            free(y_new);
        }
    }
    free(x_new);
}

void freeObsTable(ObsTable *obs_table)
{
    obs_table->obs_n = 0;
    free(obs_table->obs_records);
}