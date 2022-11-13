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
            gtime_t time;

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
}

void imputeTime(ObsTable *obs_table)
{

}