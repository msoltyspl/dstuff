#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <limits.h>

#include "bfile.h"

#ifdef __MINGW_H
#define _TFR(arg) (arg)
#else
#define _TFR(expression) \
  (__extension__                                                              \
    ({ long int __result;                                                     \
       do __result = (long int) (expression);                                 \
       while (__result == -1L && errno == EINTR);                             \
       __result; }))
#define O_BINARY 0
#endif

#define CHUNK_SIZE (16*1048576)
//#define CHUNK_SIZE 16384

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static int read_wrapper(int desc, char *buffer, size_t siz)
{
	size_t total = 0u;
	ssize_t check;

	while(total < siz) {
                check=_TFR(read(desc, buffer+total, siz-total));
		if(check<0)
			return  -1;
		if(check == 0)
			return total;
		total+=check;
	}
	return total;
}

static int write_wrapper(int desc, const char *buffer, size_t siz)
{
	size_t total = 0u;
	ssize_t check;

	while(total < siz) {
                check=_TFR(write(desc, buffer+total, siz-total));
		if(check<0)
			return -1;
		total+=check;
	}
	return total;
}
#if 0
off_t bfsize(BFdesc *bf)
{
	off_t ret = -1;
	struct stat statbuf;

	if(!bf)
		return -1;

	if((ret = fstat(bf->desc,&statbuf))<0 || !S_ISREG(statbuf.st_mode))
		EXCEPT;

	ret = statbuf.st_size;

	EXCEPT_LABEL

	return ret;
}
#endif
int bfread(BFdesc *bf, char *dbuf, int dsiz)
{
	int doff = 0, tsiz;	

//	if(!(bf && bf->buf))
//		return -1;

	while(doff < dsiz) {
		tsiz = MIN(dsiz - doff, bf->siz - bf->off);

		memcpy(dbuf + doff, bf->buf + bf->off, tsiz);

		bf->off += tsiz;
		doff += tsiz;

		if(bf->off == bf->siz) { //sbuf depleted, read new chunk
			tsiz = read_wrapper(bf->desc, bf->buf, bf->chunk);
			if(tsiz < 0) {
				fputs("bfread: error\n",stderr);
				return tsiz; //error
			}
//			if(tsiz == 0 && bf->off == 0)
//				return 0;
			bf->off = 0;
			bf->siz = tsiz;
			if(tsiz == 0)
				break;
		}
	}	
	return doff;
}

int bfwrite(BFdesc *bf, const char *sbuf, int ssiz)
{
	int soff = 0, tsiz;	

//	if(!(bf && bf->buf))
//		return -1;

	while(soff < ssiz) {
		tsiz = MIN(ssiz - soff, bf->chunk - bf->off);

		memcpy(bf->buf + bf->off, sbuf + soff, tsiz);

		bf->off += tsiz;
		soff += tsiz;

		if(bf->off == bf->chunk) { //dbuf full, write current chunk
			tsiz = write_wrapper(bf->desc, bf->buf, bf->off);
			if(tsiz < 0) {
				fputs("bfwrite: error\n",stderr);
				return tsiz; //error. this one can't write less than required
			}
			bf->off = 0;
		}

	}	
	return soff;
}

int bfflush(BFdesc *bf)
{
	int tsiz = 0;
	if(bf->off > 0) { //something to flush
		tsiz = write_wrapper(bf->desc, bf->buf, bf->off);
		if(tsiz > 0)
			bf->off = 0;
	}
	return tsiz;
}

BFdesc *bfopen(enum BFmode mode, const char* fname, int chunk)
{
	BFdesc *bf, *ret = NULL;
	int desc;//, st;
	struct stat statbuf;

	if(!(bf = (BFdesc *)malloc(sizeof(BFdesc))))
		EXCEPT;

	memset(bf,0,sizeof(BFdesc));
	if(chunk > 0 && chunk < 256*1048576)
		bf->chunk = chunk;
	else
		bf->chunk = CHUNK_SIZE;

	if(!(bf->buf = (char *)malloc(bf->chunk))) {
		fputs("bfopen: chunk too big\n",stderr);
		EXCEPT;
	}

	switch(mode) {
		case r: desc = _TFR(open(fname, O_RDONLY | O_BINARY));  break;
		case w: desc = _TFR(open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666)); break;
		default: EXCEPT;
	}

	if(desc < 0)
		EXCEPT;

	if(fstat(desc,&statbuf) < 0 || !S_ISREG(statbuf.st_mode))
		EXCEPT;

	bf->siz = 0;
	bf->off = 0;
	bf->mode = mode;
	bf->desc = desc;
	bf->fsize = statbuf.st_size;

	strncpy(bf->fname,fname,PATH_MAX);
	bf->fname[PATH_MAX] = 0;

	ret = bf;
EXCEPT_LABEL
	if(!ret && bf) {
		if(bf->buf)
			free(bf->buf);
		free(bf);
	}
	return ret;
}

int bfclose(BFdesc *bf)
{
	int ret = 0;
	if(!bf)
		return -1;
	if(bf->mode == w)
		ret = bfflush(bf);
	_TFR(close(bf->desc));

	if(bf->buf)
		free(bf->buf);
	free(bf);

	return ret >= 0 ? 0 : -1;
}
