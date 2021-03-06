#ifndef _LOKI_COMPRESS_H_
#define _LOKI_COMPRESS_H_

#define COMPRESS_GZIP 0
#define COMPRESS_BZIP2 1
#define COMPRESS_ZIP 2
#define COMPRESS_COMPRESS 3
#define COMPRESS_NONE 4

struct lk_compress {
  char *comp_path[COMPRESS_NONE][2];
  char *compress_suffix[COMPRESS_NONE];
  int default_compress;
};


struct lk_compress *init_compress(void);
char *find_prog(const char *,const char *);

#endif
