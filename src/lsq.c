#include "uwblib.h"

static void leastSquareOneEpochOneTag(const AnchorTable *anchor_table, const ObsTable *obs_table_epoch, const gtime_t epoch, const int tag_id, ResTable *res_table)
{
    int res_i = 0;
    // filter out obs of this tag_id
    int n = 0;
    for (int obs_i = 0; obs_i < obs_table_epoch->obs_n; obs_i++)
    {
        if (obs_table_epoch->obs_records[obs_i].tag_id == tag_id)
        {
            n++;
        }
    }

    if (n < 3)
    {
        char line[MAX_LINE_LEN] = "";
        time2str(epoch, line, 1);
        printf("WARNING: tag_id=%d no enough data (n=%d) at epoch %s", tag_id, n, line);
    }

    gsl_matrix *O = gsl_matrix_alloc(n, 1);
    gsl_matrix *B = gsl_matrix_alloc(n, 3);
    gsl_matrix *X = gsl_matrix_alloc(3, 1);
    gsl_matrix_set_zero(X); // TODO: read X approx value from res_table
    gsl_matrix *C = gsl_matrix_alloc(n, 1);
    gsl_matrix *P = gsl_matrix_alloc(n, n);
    gsl_matrix_set_identity(P);
    gsl_matrix *x = gsl_matrix_alloc(3, 1);
    gsl_matrix *Dx = gsl_matrix_alloc(3, 3);

    for (int iter = 1; iter <= 3; iter++)
    {
        int i = 0;
        for (int obs_i = 0; obs_i < obs_table_epoch->obs_n; obs_i++)
        {

            if (obs_table_epoch->obs_records[obs_i].tag_id == tag_id)
            {
                gsl_matrix_set(O, i, 0, obs_table_epoch->obs_records[obs_i].distance / 100.0); // cm-> m
                double tag_n = gsl_matrix_get(X, 0, 0);
                double tag_e = gsl_matrix_get(X, 1, 0);
                double tag_u = gsl_matrix_get(X, 2, 0);

                int anchor_id = obs_table_epoch->obs_records[obs_i].anchor_id;
                double anchor_n = 0;
                double anchor_e = 0;
                double anchor_u = 0;
                getAnchorPosByID(anchor_table, anchor_id, &anchor_n, &anchor_e, &anchor_u);

                double dist = sqrt((tag_n - anchor_n) * (tag_n - anchor_n) + (tag_e - anchor_e) * (tag_e - anchor_e) + (tag_u - anchor_u) * (tag_u - anchor_u));
                gsl_matrix_set(C, i, 0, dist);

                gsl_matrix_set(B, i, 0, (tag_n - anchor_n) / dist);
                gsl_matrix_set(B, i, 1, (tag_e - anchor_e) / dist);
                gsl_matrix_set(B, i, 2, (tag_u - anchor_u) / dist);

                i++;
            }
        }

        printf("Iter:%d\n", iter);
        printf("O Matrix:\n");
        printfMatrix(O);
        printf("C Matrix:\n");
        printfMatrix(C);

        gsl_matrix_sub(O, C);
        printf("l Matrix:\n");
        printfMatrix(O);
        printf("B Matrix:\n");
        printfMatrix(B);

        solveLSQ(B, P, O, x, Dx);
        printf("x Matrix:\n");
        printfMatrix(x);

        printf("Dx Matrix:\n");
        printfMatrix(Dx);

        gsl_matrix_add(X, x);
        printf("X Matrix:\n");
        printfMatrix(X);
    }

    res_table->res_records[res_i].tag_id = tag_id;
    res_table->res_records[res_i].time = epoch;
    res_table->res_records[res_i].n = gsl_matrix_get(X, 0, 0);
    res_table->res_records[res_i].e = gsl_matrix_get(X, 1, 0);
    res_table->res_records[res_i].u = gsl_matrix_get(X, 2, 0);
    for (int D_i = 0; D_i < 9; D_i++)
    {
        res_table->res_records[res_i].D[D_i] = gsl_matrix_get(Dx, D_i / 3, D_i % 3);
    }
    res_i++;

    gsl_matrix_free(O);
    gsl_matrix_free(B);
    gsl_matrix_free(X);
    gsl_matrix_free(C);
    gsl_matrix_free(P);
    gsl_matrix_free(x);
    gsl_matrix_free(Dx);
}

void leastSquareOneEpoch(const AnchorTable *anchor_table, const ObsTable *obs_table, const gtime_t epoch, const int *unique_tag_ids, const int unique_tag_n, ResTable *res_table)
{
    ObsTable obs_table_epoch = {0};
    int reserve_n = 2 * ANCHOR_MAX_ID_N * unique_tag_n; // times 2 to reserve for duplicate measurements
    obs_table_epoch.obs_records = malloc(reserve_n * sizeof(ObsRecord));

    // filter out obs in this epoch
    int obs_small_i = 0;
    for (int obs_i = 0; obs_i < obs_table->obs_n; obs_i++)
    {
        if (fabs(timediff(epoch, obs_table->obs_records[obs_i].time)) < 0.1)
        {
            memcpy(&(obs_table_epoch.obs_records[obs_small_i]), &(obs_table->obs_records[obs_i]), sizeof(ObsRecord));
            obs_small_i++;
        }
    }
    obs_table_epoch.obs_n = obs_small_i;

    // for each tag
    for (int tag_i = 0; tag_i < unique_tag_n; tag_i++)
    {
        int tag_id = unique_tag_ids[tag_i];
        leastSquareOneEpochOneTag(anchor_table, &obs_table_epoch, epoch, tag_id, res_table);
    }

    freeObsTable(&obs_table_epoch);
}