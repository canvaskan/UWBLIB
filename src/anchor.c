#include "uwblib.h"

void readAnchorTable(const Config *config, AnchorTable *anchor_table)
{
    FILE *fp = fopen(config->anchor_file, "r");
    if (!fp)
    {
        printf("ERROR: readAnchorTable cannot open file!\n");
        exit(EXIT_FAILURE);
    }

    // drop header
    char line[MAX_LINE_LEN] = "";
    fgets(line, MAX_LINE_LEN, fp);

    // read record
    for (int i = 0; i < ANCHOR_MAX_N; i++)
    {
        fgets(line, MAX_LINE_LEN, fp);

        int n = sscanf(line, "%s %lf %lf %lf %d %d",
                       (anchor_table->anchor_records[i].anchor_name),
                       &(anchor_table->anchor_records[i].n),
                       &(anchor_table->anchor_records[i].e),
                       &(anchor_table->anchor_records[i].u),
                       &(anchor_table->anchor_records[i].ants[0]),
                       &(anchor_table->anchor_records[i].ants[1]));

        if (n != 6)
        {
            printf("ERROR: readAnchorTable format error!\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
}

void getAnchorPosByID(const AnchorTable *anchor_table, const int anchor_id, double *n, double *e, double *u)
{
    int found = 0;
    for(int i=0;i<ANCHOR_MAX_N;i++)
    {
        if(anchor_table->anchor_records[i].anchor_name[0] == '\0')
        {
            continue;
        }
        for(int j=0;j<ANCHOR_MAX_ANT_N;j++)
        {
            if(anchor_table->anchor_records[i].ants[j] == anchor_id)
            {
                *n = anchor_table->anchor_records[i].n;
                *e = anchor_table->anchor_records[i].e;
                *u = anchor_table->anchor_records[i].u;
                found =1;
                break;
            }
        }
        if(found)
        {
            break;
        }
    }
    if(!found)
    {
        printf("ERROR: getAnchorPosByID not found!\n");
        exit(EXIT_FAILURE);
    }
}