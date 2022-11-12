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

#define MAX_LINE_LEN 1024

/* UTILS START ============================================*/

typedef struct
{                /* time struct */
    time_t time; /* time (s) expressed by standard time_t */
    double sec;  /* fraction of second under 1 s */
} gtime_t;

/// @brief chop trailing spaces and '\n', see '#' as comment
/// @param str str to be chop
void chop(char *str);

int str2time(const char *s, int i, int n, gtime_t *t);
gtime_t epoch2time(const double *ep);

/* UTILS END ==============================================*/

/* CONFIG START ===========================================*/
#define CONFIG_MAX_OBS_FILE_N 4
#define CONFIG_POS_METHOD_LSQ 1
#define CONFIG_POS_METHOD_STATIC_KF 2
#define CONFIG_POS_METHOD_KINEMATIC_KF 3

typedef struct
{
    char config_file[MAX_LINE_LEN];
    char obs_files[CONFIG_MAX_OBS_FILE_N][MAX_LINE_LEN];
    char anchor_file[MAX_LINE_LEN];
    int pos_method;
    char result_file[MAX_LINE_LEN];
} Config;

void readConfig(const char *config_file, Config *config);

/* CONFIG END =============================================*/

/* ANCHOR START ===========================================*/
#define ANCHOR_MAX_NAME_LEN 10
#define ANCHOR_MAX_ANT_N 2
#define ANCHOR_MAX_N 4
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

void readAnchorTable(const Config *config, AnchorTable *anchor_table);

/* ANCHOR END =============================================*/

/* DECODE START ===========================================*/
#define DECODE_MAX_OBS_RECORD 28800

typedef struct
{
    gtime_t time;
    int anchor_id;
    int tag_id;
    int distance; // cm
    int battery;  // 1%-100%
    int SOS;      // 0:No, 1:Yes
    int mobile;   // 0:No, 1:Moving
} ObsRecord;

typedef struct
{
    ObsRecord obs_records[DECODE_MAX_OBS_RECORD];
} ObsTable;

typedef struct
{
    gtime_t time;
    int anchor_id;
    int status; // 0:Normal
} HeartbeatRecord;

typedef struct
{
    HeartbeatRecord heartbeat_records[DECODE_MAX_OBS_RECORD];
} HeartbeatTable;

/// @brief decode binary obs and save to .txt readable format
/// @param config 
void decodeBinaryObs(const Config *config);

/* DECODE END =============================================*/

#endif