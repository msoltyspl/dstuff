#ifndef __bfile_h__
#define __bfile_h__
//#include <fcntl.h>
#include <sys/param.h>

enum BFmode { c = 0, r, w };

typedef struct _BFdesc {
	int desc, siz, off, chunk;
	off_t fsize;
	enum BFmode mode;
	char fname[MAXPATHLEN+1];
	char *buf;
} BFdesc;

int bfread(BFdesc *bf, char *dbuf, int dsiz);
int bfwrite(BFdesc *bf, const char *sbuf, int ssiz);
BFdesc *bfopen(enum BFmode mode, const char* fname, int chunk);
//BFdesc *bfopen(enum BFmode mode, const char* fname);
int bfclose(BFdesc *bf);
int bfflush(BFdesc *bf);
//off_t bfsize(BFdesc *bf);

#endif
