#ifndef _LIBHDR_H_
#define _LIBHDR_H_

#include "utils.h"

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

int child_open(const int,const char *,const char *);
int child_open1(const int,const char *,const char *,const char *);
int child_open_rw(int [2],const char *,char *const argv[]);
FILE *_open_readfile(const char *,int *,const struct lk_compress *,int);
#define open_readfile_and_check(a,b,c) _open_readfile((a),(b),(c),1);
#define open_readfile(a,b,c) _open_readfile((a),(b),(c),0);
int getseed(const char *);
int writeseed(const char *,const int);
int dumpseed(FILE *,const int);
int mkbackup(const char *,int);
int bindumpseed(FILE *);
int binreadseed(FILE *,char *);
char *my_strsep(char **,const char *);

extern double cumchic(double,int);
extern double cumchn(double,int,double);
extern double critchi(double,int);
extern double critchn(double,int,double);

#define cumchi(x,df) (1.0-cumchic((x),(df)))
#define cnorm(x) (.5*(1.0+erf((x)*M_SQRT1_2)))

#endif
