
#include "uwblib.h"

void readConfig(const char *config_file, Config *config)
{
    FILE *fp = fopen(config_file, "r");
    char line[MAX_LINE_LEN] = "";
    int obs_i = 0;
    char *opt = NULL;

    if(!fp)
    {
        system("pwd");
        printf("ERROR: readConfig cannot open config file %s!\n", config_file);
        exit(EXIT_FAILURE);
    }

    while(fgets(line, MAX_LINE_LEN, fp))
    {
        if(strstr(line, "obs_file"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            strcpy(config->obs_files[obs_i], opt);
            obs_i++;
        }
        else if (strstr(line, "anchor_file"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            strcpy(config->anchor_file, opt);
        }
        else if (strstr(line, "start_time"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            str2time_json(opt, 0, 19, &(config->start_time));
        }
        else if (strstr(line, "end_time"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            str2time_json(opt, 0, 19, &(config->end_time));
        }
        else if (strstr(line, "interval_sec"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            config->interval_sec = atof(opt);
        }
        else if (strstr(line, "pos_method"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            config->pos_method = atoi(opt);
        }
        else if (strstr(line, "result_file"))
        {
            opt = strchr(line, '=') + 1;
            chop(opt);
            strcpy(config->result_file, opt);
        }        
    }

    fclose(fp);
    return;
}