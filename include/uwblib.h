/**
 * @file uwblib.h
 * @author Haoyu Kan
 * @brief A program for UWB indoor positioning
 * @version 0.1
 * @date 2022-11-12
 *
 * @copyright Copyright (c) 2022
 *
 * @ref RTKLIB
 */

#ifndef UWBLIB_H_
#define UWBLIB_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_linalg.h>

#define MAX_LINE_LEN 1024

/* UTILS START ============================================*/

typedef struct
{                /* time struct */
    time_t time; /* time (s) expressed by standard time_t */
    double sec;  /* fraction of second under 1 s */
} gtime_t;

typedef struct
{
    gtime_t start_time;
    gtime_t end_time;
    int n;
    double interval_sec;
    gtime_t *times;
} gtimeSeries;

/// @brief chop trailing spaces and '\n', see '#' as comment
/// @param str IO str to be chop
void chop(char *str);

int str2time_json(const char *s, int i, int n, gtime_t *t);
int str2time(const char *s, int i, int n, gtime_t *t);
gtime_t timeadd(gtime_t t, double sec);
extern double timediff(gtime_t t1, gtime_t t2);
gtime_t epoch2time(const double *ep);

/// @brief make evenly-spaced time series, use MALLOC!!!, free by freeTimeSeries
/// @param time_series  O
/// @param start_time   I
/// @param end_time     I
/// @param interval_sec I
void makeTimeSeries(gtimeSeries *time_series, gtime_t start_time, gtime_t end_time, double interval_sec);
void freeTimeSeries(gtimeSeries *time_series);

/// @brief linear interpolation, GSL wrapper
/// @param x      I data point x coor
/// @param y      I data point y coor
/// @param n      I number of data point
/// @param x_new  I interp point x coor
/// @param y_new  O interp point y coor
/// @param n_new  I number of interp point
void linearInterp(const double *x, const double *y, const double n, const double *x_new, double *y_new, const double n_new);

/// @brief inverse matrix using LU decomposition
/// @param mat matrix to invert
/// @return inverse matrix, need gsl_matrix_free!!!
gsl_matrix *inverseMatrix(const gsl_matrix *mat);

/// @brief solve lsq: V = B@x - l
/// @param B  I coeffient or design matrix
/// @param P  I power or weight matrix
/// @param l  I OMC, observation minus computation
/// @param x  O estimated params, x = inv(B.T@P@B)@(B.T@P@l)
/// @param Dx O estimated covariance, Dx = inv(B.T@P@B)
void solveLSQ(const gsl_matrix*B, const gsl_matrix*P, const gsl_matrix*l, gsl_matrix*x, gsl_matrix*Dx);

/* UTILS END ==============================================*/

/* CONFIG START ===========================================*/
#define CONFIG_MAX_OBS_FILE_N 10
#define CONFIG_POS_METHOD_LSQ 1
#define CONFIG_POS_METHOD_STATIC_KF 2
#define CONFIG_POS_METHOD_KINEMATIC_KF 3

typedef struct
{
    char config_file[MAX_LINE_LEN];
    char obs_files[CONFIG_MAX_OBS_FILE_N][MAX_LINE_LEN];
    char anchor_file[MAX_LINE_LEN];
    gtime_t start_time;
    gtime_t end_time;
    double interval_sec;
    int pos_method;
    char result_file[MAX_LINE_LEN];
} Config;

/// @brief read config file
/// @param config_file I config file path
/// @param config      O config struct
void readConfig(const char *config_file, Config *config);

/* CONFIG END =============================================*/

/* ANCHOR START ===========================================*/
#define ANCHOR_MAX_NAME_LEN 10
#define ANCHOR_MAX_ANT_N 2
#define ANCHOR_MAX_N 4
#define ANCHOR_MAX_ID_N ANCHOR_MAX_N *ANCHOR_MAX_ANT_N
typedef struct
{
    char anchor_name[ANCHOR_MAX_NAME_LEN];
    double n, e, u;
    int ants[ANCHOR_MAX_ANT_N];
} AnchorRecord;

typedef struct
{
    AnchorRecord anchor_records[ANCHOR_MAX_N];
} AnchorTable;

/// @brief read Anchor Table, format: ANCHOR   N      E     U    ANT1  ANT2
/// @param config       I config struct
/// @param anchor_table O anchor table struct
void readAnchorTable(const Config *config, AnchorTable *anchor_table);

/* ANCHOR END =============================================*/

/* DECODE START ===========================================*/
#define DECODE_ASCII_OBS_SUFFIX ".obs"
#define DECODE_ASCII_HEARTBEAT_SUFFIX ".heartbeat"

/// @brief decode binary obs and save to readable format,
/// filename suffix by DECODE_ASCII_OBS_SUFFIX and DECODE_ASCII_HEARTBEAT_SUFFIX
/// @param config I config struct
void decodeBinaryObs(const Config *config);

/* DECODE END =============================================*/

/* LOADOBS START ==========================================*/
#define LOADOBS_MAX_RECORD_N 86400
#define LOADOBS_MAX_IMPUTE_SPAN 3
#define LOADOBS_MAX_TAG_N 4
#define LOADOBS_MAX_INTERP_RECORD_N 86400

typedef struct
{
    gtime_t time;
    int anchor_id;
    int tag_id;
    int distance; // cm
    int battery;  // 1%-100%
    int SOS;      // 0:No, 1:Yes
    int mobile;   // 0:No, other:Moving

    int is_interp;// 0:No, 1:Yes
} ObsRecord;

typedef struct
{
    int obs_n;
    ObsRecord obs_records[LOADOBS_MAX_RECORD_N];
} ObsTable;

typedef struct
{
    gtime_t time;
    int anchor_id;
    int status; // 0:Normal
} HeartbeatRecord;

typedef struct
{
    HeartbeatRecord heartbeat_records[LOADOBS_MAX_RECORD_N];
} HeartbeatTable;

/// @brief load Obs from decoded obs file
/// @param config     I config struct
/// @param obs_table  O all obs stored unsorted, because time may be missing
void loadObsTable(const Config *config, ObsTable *obs_table);

/// @brief impute (fill) time using before and after record's time,
/// only use nearest LOADOBS_MAX_IMPUTE_SPAN records
/// @param obs_table  IO obs table to be imputed
void imputeTime(ObsTable *obs_table);

/// @brief sort obs_table by time, old->new, inplace
/// @param obs_table IO ObsTable
void sortByTime(ObsTable *obs_table);

/// @brief find unique anchor id in obs table
/// @param obs_table          I obs table
/// @param unique_anchor_ids  O unique anchor ids
/// @return                     number of unique anchor ids
int uniqueAnchorID(const ObsTable *obs_table, int *unique_anchor_ids);

/// @brief find unique tag id in obs table
/// @param obs_table          I obs table
/// @param unique_tag_ids     O unique tag ids
/// @return                     number of unique tag ids
int uniqueTagID(const ObsTable *obs_table, int *unique_tag_ids);

/// @brief after imputed and sorted, every anchor-tag pair in
/// obs table should be interp to evenly-spaced epochs indicated by gtimeSeries,
/// gtimeSeries can be created by makeTimeSeries, and freed by FreeTimeSeries
/// @param obs_table         I   imputed and sorted but uneven obs
/// @param obs_table_new     O   evenly-spaced obs
/// @param time_series       I   evenly-spaced time series
/// @param unique_anchor_ids I   unique anchor ids
/// @param unique_anchor_n   I   number of unique anchor ids
/// @param unique_tag_ids    I   unique tag ids
/// @param unique_tag_n      I   number of unique tag ids
void interpObsTable(const ObsTable *obs_table, ObsTable *obs_table_new, const gtimeSeries *time_series,
                    const int *unique_anchor_ids, const int unique_anchor_n, const int *unique_tag_ids, const int unique_tag_n);

/* LOADOBS END ============================================*/

/* LSQ START ==============================================*/

typedef struct 
{
    gtime_t time;
    int tag_id;
    double n,e,u;
    double D[3*3];
} ResRecord;

typedef struct 
{
    int n;
    ResRecord res_records[LOADOBS_MAX_INTERP_RECORD_N];
} ResTable;

/// @brief Process all tag one epoch data with LSQ algorithm, store result in res_table
/// @param obs_table_new  I obs data
/// @param epoch          I chosen epoch
/// @param unique_tag_ids I chosen tag id
/// @param unique_tag_n   I number of chosen tag
/// @param res_table      O res table
void leastSquareOneEpoch(const ObsTable *obs_table, const gtime_t epoch, const int *unique_tag_ids, const int unique_tag_n, ResTable* res_table);

/// @brief Process one tag one epoch data with LSQ algorithm, store result in res_table
/// @param obs_table_new I obs data
/// @param epoch         I chosen epoch
/// @param tag_id        I chosen tag id
/// @param res_table     O res table
void leastSquareOneEpochOneTag(const ObsTable *obs_table, const gtime_t epoch, const int tag_id, ResTable* res_table);


/* LSQ END ================================================*/


#endif