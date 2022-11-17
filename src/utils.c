#include "uwblib.h"

extern void chop(char *str)
{
    char *p;
    if ((p = strchr(str, '#')))
        *p = '\0'; /* comment */
    for (p = str + strlen(str) - 1; p >= str && !isgraph((int)*p); p--)
        *p = '\0';
}
/* string to time --------------------------------------------------------------
 * convert substring in string to gtime_t struct
 * args   : char   *s        I   string ("... yyyy-mm-dd hh:mm:ss ...")
 *          int    i,n       I   substring position and width
 *          gtime_t *t       O   gtime_t struct
 * return : status (0:ok,0>:error)
 *-----------------------------------------------------------------------------*/
extern int str2time_json(const char *s, int i, int n, gtime_t *t)
{
    double ep[6];
    char str[256], *p = str;

    if (i < 0 || (int)strlen(s) < i || (int)sizeof(str) - 1 < i)
        return -1;
    for (s += i; *s && --n >= 0;)
        *p++ = *s++;
    *p = '\0';
    if (sscanf(str, "%lf-%lf-%lf %lf:%lf:%lf", ep, ep + 1, ep + 2, ep + 3, ep + 4, ep + 5) < 6)
        return -1;
    if (ep[0] < 100.0)
        ep[0] += ep[0] < 80.0 ? 2000.0 : 1900.0;
    *t = epoch2time(ep);
    return 0;
}
/* string to time --------------------------------------------------------------
 * convert substring in string to gtime_t struct
 * args   : char   *s        I   string ("... yyyy mm dd hh mm ss ...")
 *          int    i,n       I   substring position and width
 *          gtime_t *t       O   gtime_t struct
 * return : status (0:ok,0>:error)
 *-----------------------------------------------------------------------------*/
extern int str2time(const char *s, int i, int n, gtime_t *t)
{
    double ep[6];
    char str[256], *p = str;

    if (i < 0 || (int)strlen(s) < i || (int)sizeof(str) - 1 < i)
        return -1;
    for (s += i; *s && --n >= 0;)
        *p++ = *s++;
    *p = '\0';
    if (sscanf(str, "%lf %lf %lf %lf %lf %lf", ep, ep + 1, ep + 2, ep + 3, ep + 4, ep + 5) < 6)
        return -1;
    if (ep[0] < 100.0)
        ep[0] += ep[0] < 80.0 ? 2000.0 : 1900.0;
    *t = epoch2time(ep);
    return 0;
}
/* convert calendar day/time to time -------------------------------------------
 * convert calendar day/time to gtime_t struct
 * args   : double *ep       I   day/time {year,month,day,hour,min,sec}
 * return : gtime_t struct
 * notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
 *-----------------------------------------------------------------------------*/
extern gtime_t epoch2time(const double *ep)
{
    const int doy[] = {1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
    gtime_t time = {0};
    int days, sec, year = (int)ep[0], mon = (int)ep[1], day = (int)ep[2];

    if (year < 1970 || 2099 < year || mon < 1 || 12 < mon)
        return time;

    /* leap year if year%4==0 in 1901-2099 */
    days = (year - 1970) * 365 + (year - 1969) / 4 + doy[mon - 1] + day - 2 + (year % 4 == 0 && mon >= 3 ? 1 : 0);
    sec = (int)floor(ep[5]);
    time.time = (time_t)days * 86400 + (int)ep[3] * 3600 + (int)ep[4] * 60 + sec;
    time.sec = ep[5] - sec;
    return time;
}
/* add time --------------------------------------------------------------------
 * add time to gtime_t struct
 * args   : gtime_t t        I   gtime_t struct
 *          double sec       I   time to add (s)
 * return : gtime_t struct (t+sec)
 *-----------------------------------------------------------------------------*/
extern gtime_t timeadd(gtime_t t, double sec)
{
    double tt;

    t.sec += sec;
    tt = floor(t.sec);
    t.time += (int)tt;
    t.sec -= tt;
    return t;
}
/* time difference -------------------------------------------------------------
 * difference between gtime_t structs
 * args   : gtime_t t1,t2    I   gtime_t structs
 * return : time difference (t1-t2) (s)
 *-----------------------------------------------------------------------------*/
extern double timediff(gtime_t t1, gtime_t t2)
{
    return difftime(t1.time, t2.time) + t1.sec - t2.sec;
}
/* time to calendar day/time ---------------------------------------------------
 * convert gtime_t struct to calendar day/time
 * args   : gtime_t t        I   gtime_t struct
 *          double *ep       O   day/time {year,month,day,hour,min,sec}
 * return : none
 * notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
 *-----------------------------------------------------------------------------*/
extern void time2epoch(gtime_t t, double *ep)
{
    const int mday[] = {/* # of days in a month */
                        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
                        31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days, sec, mon, day;

    /* leap year if year%4==0 in 1901-2099 */
    days = (int)(t.time / 86400);
    sec = (int)(t.time - (time_t)days * 86400);
    for (day = days % 1461, mon = 0; mon < 48; mon++)
    {
        if (day >= mday[mon])
            day -= mday[mon];
        else
            break;
    }
    ep[0] = 1970 + days / 1461 * 4 + mon / 12;
    ep[1] = mon % 12 + 1;
    ep[2] = day + 1;
    ep[3] = sec / 3600;
    ep[4] = sec % 3600 / 60;
    ep[5] = sec % 60 + t.sec;
}
/* time to string --------------------------------------------------------------
 * convert gtime_t struct to string
 * args   : gtime_t t        I   gtime_t struct
 *          char   *s        O   string ("yyyy/mm/dd hh:mm:ss.ssss")
 *          int    n         I   number of decimals
 * return : none
 *-----------------------------------------------------------------------------*/
extern void time2str(gtime_t t, char *s, int n)
{
    double ep[6];

    if (n < 0)
        n = 0;
    else if (n > 12)
        n = 12;
    if (1.0 - t.sec < 0.5 / pow(10.0, n))
    {
        t.time++;
        t.sec = 0.0;
    };
    time2epoch(t, ep);
    sprintf(s, "%04.0f/%02.0f/%02.0f %02.0f:%02.0f:%0*.*f", ep[0], ep[1], ep[2],
            ep[3], ep[4], n <= 0 ? 2 : n + 3, n <= 0 ? 0 : n, ep[5]);
}

void makeTimeSeries(gtimeSeries *time_series, gtime_t start_time, gtime_t end_time, double interval_sec)
{
    time_series->n = timediff(end_time, start_time) / interval_sec;
    if (time_series->n <= 0)
    {
        printf("ERROR! makeTimeSeries timediff(end_time, start_time) / interval_sec <= 0!\n");
        exit(EXIT_FAILURE);
    }
    time_series->start_time = start_time;
    time_series->end_time = end_time;
    time_series->interval_sec = interval_sec;
    time_series->times = (gtime_t *)malloc(time_series->n * sizeof(gtime_t));
    if (time_series->times == NULL)
    {
        printf("ERROR! makeTimeSeries cannot alloc memory!\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < time_series->n; i++)
    {
        time_series->times[i] = timeadd(start_time, i * interval_sec);
    }
}

void freeTimeSeries(gtimeSeries *time_series)
{
    free(time_series->times);
    memset(time_series, 0, sizeof(gtimeSeries));
}

void linearInterp(const double *x, const double *y, const double n, const double *x_new, double *y_new, const double n_new)
{
    gsl_interp_accel *acc = gsl_interp_accel_alloc();
    gsl_spline *spline = gsl_spline_alloc(gsl_interp_linear, n);
    gsl_spline_init(spline, x, y, n);
    for (int i = 0; i < n_new; i++)
    {
        double xi = x_new[i];
        y_new[i] = gsl_spline_eval(spline, xi, acc);
    }
    gsl_spline_free(spline);
    gsl_interp_accel_free(acc);
}

void inverseMatrix(const gsl_matrix *mat, gsl_matrix * inv)
{
    if (mat->size1 != mat->size2)
    {
        printf("ERROR: inverseMatrix need square matrix\n");
        exit(EXIT_FAILURE);
    }

    gsl_matrix *copy = gsl_matrix_alloc(mat->size1, mat->size2);
    gsl_matrix_memcpy(copy, mat); // LU decomp is IO

    gsl_permutation *p = gsl_permutation_alloc(copy->size1);
    int s;

    // Compute the LU decomposition of this matrix
    gsl_linalg_LU_decomp(copy, p, &s);

    // Compute the  inverse of the LU decomposition
    gsl_linalg_LU_invert(copy, p, inv);

    gsl_permutation_free(p);
    gsl_matrix_free(copy);
}

void printfMatrix(const gsl_matrix *mat)
{
    for (int i = 0; i < mat->size1; i++)
    {
        for (int j = 0; j < mat->size2; j++)
        {
            printf("% g\t", gsl_matrix_get(mat, i, j));
        }
        printf("\n");
    }
}

void solveLSQ(const gsl_matrix *B, const gsl_matrix *P, const gsl_matrix *l, gsl_matrix *x, gsl_matrix *Dx)
{
    gsl_matrix *BTP = gsl_matrix_alloc(B->size2, P->size2);
    gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, B, P, 0.0, BTP);

    gsl_matrix *BTPB = gsl_matrix_alloc(BTP->size1, B->size2);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, BTP, B, 0.0, BTPB);
    printf("BTPB Matrix\n");
    printfMatrix(BTPB);

    inverseMatrix(BTPB, Dx);
    printf("BTPBinv Matrix\n");
    printfMatrix(Dx);

    gsl_matrix *BTPl = gsl_matrix_alloc(BTP->size1, l->size2);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, BTP, l, 0.0, BTPl);
    printf("BTPl Matrix\n");
    printfMatrix(BTPl);

    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Dx, BTPl, 0.0, x);

    gsl_matrix_free(BTP);
    gsl_matrix_free(BTPB);
    gsl_matrix_free(BTPl);
}

