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

gsl_matrix *inverseMatrix(const gsl_matrix *mat)
{
    if (mat->size1 != mat->size2)
    {
        printf("ERROR: inverseMatrix need square matrix\n");
        exit(EXIT_FAILURE);
    }
    gsl_permutation *p = gsl_permutation_alloc(mat->size1 * mat->size2);
    int s;

    // Compute the LU decomposition of this matrix
    gsl_linalg_LU_decomp(mat, p, &s);

    // Compute the  inverse of the LU decomposition
    gsl_matrix *inv = gsl_matrix_alloc(mat->size1, mat->size2);
    gsl_linalg_LU_invert(mat, p, inv);

    gsl_permutation_free(p);

    return inv;
}

void solveLSQ(const gsl_matrix *B, const gsl_matrix *P, const gsl_matrix *l, gsl_matrix *x, gsl_matrix *Dx)
{
    gsl_matrix *BTP = gsl_matrix_alloc(B->size2, P->size2);
    gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, B, P, 0.0, BTP);
    gsl_matrix *BTPB = gsl_matrix_alloc(BTP->size1, B->size2);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, BTP, B, 0.0, BTPB);
    Dx = inverseMatrix(BTPB);
    gsl_matrix *BTPl = gsl_matrix_alloc(BTP->size1, l->size2);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Dx, BTPl, 0.0, x);

    gsl_matrix_free(BTP);
    gsl_matrix_free(BTPB);
    gsl_matrix_free(BTPl);
}