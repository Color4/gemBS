#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

#include "config.h"
#include "utils.h"

#ifndef string_srep
#define string_srep __string_srep
struct __string_srep {
	void *__st_p;
	size_t __st_i[2];
	char *buf;
};
#endif
#ifndef _fget_bptr
#define _fget_bptr void **
#endif

#define free_fget_buffer(x) (free(*(x)))

typedef struct _sc_string {
	struct _sc_string *next;
	struct string_srep *srep;
	size_t len;
} string;

string *add_to_string(string *,char);
string *add_cstring_to_string(string *,char *);
string *addl_cstring_to_string(string *,char *,size_t);
string *make_string(size_t,int);
string *init_string(string *,char *);
string *addn_to_string(string *,char *,size_t);
string *paddn_to_string(string *,char *,size_t);
string *add_strings(string *,string *);
char *copy_cstring(string *);
char *extract_cstring(string *);
string *copy_string(string *);
string *strip_quotes(string *);
string *copy_string1(string *,string *);
void free_string(string *);
string *int_to_string(int);
string *real_to_string(double);
int string_cmp(string *,string *);
string *fget_string(FILE *,string *,_fget_bptr);
string *fget_string_gen(FILE *,string *,_fget_bptr,int);
void *get_fget_buffer(_fget_bptr,size_t *);
string *strprintf(string *,const char *, ...);
string *straprintf(string *,const char *, ...);
string *strputc(const char,string *);
#define straputc(c,s) add_to_string(s,c)
string *strputs(char *,string *);
#define straputs(p,s) add_cstring_to_string(s,p)
void free_string_lib(void);
void trim_comment(string *,int);
#define string_len(x) ((x)->len)
#define get_cstring(x) ((x)->srep->buf)

#define LOG_TEN 2.30258509299404568402

#endif
