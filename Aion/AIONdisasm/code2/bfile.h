#ifndef __bfile_h__
#define __bfile_h__

enum BFmode { c = 0, r, w };

typedef struct _BFdesc {
	int desc, siz, off, chunk;
	char *buf;
	enum BFmode mode;
	
} BFdesc;
int bfread(BFdesc *bf, char *dbuf, int dsiz);
int bfwrite(BFdesc *bf, const char *sbuf, int ssiz);
BFdesc *bfopen(enum BFmode mode, const char* fname, int chunk);
//BFdesc *bfopen(enum BFmode mode, const char* fname);
int bfclose(BFdesc *bf);
int bfflush(BFdesc *bf);

#endif
