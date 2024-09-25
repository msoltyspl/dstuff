#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
//#include <sys/types.h>
//#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <iconv.h>
#include <ctype.h>

#include "pooler.h"
#include "htmlblob.h"
#include "bfile.h"
#include "tags.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

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

static const char gs_bfr_err[]="ProcessHTMLBlob: bfread failed.\n";
static const char gs_mal_err[]="ProcessHTMLBlob: malloc failed.\n";
static const char gs_pool_err[]="ProcessHTMLBlob: pooler failed.\n";
static const char gs_dec_err[]="ProcessHTMLBlob: decrypt failed.\n";

//static const char gs_xml16tag[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";

//imports
//extern POOLER *g_pool_dynamic;

//exports
uint8_t *g_htmlconvptr = NULL;
int g_htmlconvlen = 0;//, g_htmlconvoff = 0;

static int DecryptHTMLBlob(uint8_t *buf, int len, const char *fname)
{
	uint8_t *ptr = buf;
	uint32_t seed, xorval, tmp;
	int nlen, pos, dotpos, chr, i, cnt;
	char nameseed[PATH_MAX+1];

	if(len < 2)
		return 0;//empty file with initial tag only

	pos = nlen = strlen(fname);
	dotpos = -1;

	while(--pos >= 0) {
		chr = fname[pos];
		if (chr == '\\' || chr == '/')	//slash or backslash - we back off at first one
			break;
		chr = tolower(chr);
		if (dotpos < 0 && chr == '.')	{ //record last .
			dotpos = pos;
		}
	}
	pos++;

	nlen = dotpos < 0 ? nlen - pos : dotpos - pos;

//	printf("--------- %s %d %d\n",fname,pos,nlen);

	memcpy(nameseed, fname+pos, nlen);
	nameseed[nlen] = '\0';

	seed = 0;
	xorval = 0;
	tmp = 0;
	ptr = buf + 1;

	for(i = 0; i < nlen; i++) {

		seed = i + (nameseed[i] & 0xf);
		tmp += seed;
		xorval ^= seed;
	}

        cnt = len - 1;
	while(cnt--) {
		tmp = xorval ^ (tmp + 0x1d);
		xorval += 3;
		*ptr ^= (uint8_t)tmp;
		ptr++;
	}

	//verify
	if( buf[1] != 0x81)
		return -1;

	return 0;
}

int ProcessHTMLBlob(BFdesc *fin, int convr)
{
	int ret = -1, i;
	unsigned int bom = 0;
	iconv_t cd = (iconv_t)(-1);
	size_t nconv, icr, inlen, outlen;

	uint8_t *blobptr = NULL;
	uint8_t *inptr = NULL, *outptr = NULL;

	if(fin->fsize < 2)
		return 0; //nothing to do, just close the file

	if(!(blobptr = (uint8_t*)malloc(fin->fsize))) {
		fputs(gs_mal_err,stderr);
		EXCEPT;
	}
	if(!(g_htmlconvptr = (uint8_t*)PoolerAssign(g_pool_dynamic, fin->fsize))) {
		fputs(gs_pool_err,stderr);
		EXCEPT;
	}

	blobptr[0] = 0x81;
        if((bfread(fin, (char *)(blobptr+1), fin->fsize - 1)) != fin->fsize - 1) {
		fputs(gs_bfr_err,stderr);
		EXCEPT;
	}

	printf("File size (including htmlblob tags): %u\n", (uintptr_t)(fin->fsize));

	if(DecryptHTMLBlob(blobptr, fin->fsize, fin->fname)<0) {
		fputs(gs_dec_err,stderr);
		EXCEPT;
	}

	if(fin->fsize > 4)
		bom = *(uint16_t*)(blobptr + 2);
	if(!convr && fin->fsize > 4 && (bom == g_bomLE || bom == g_bomBE)) {
		//convert to utf-8

		inptr = blobptr + 4;
		inlen = outlen = fin->fsize - 4;
		outptr = g_htmlconvptr;
		nconv = 0;

		cd = iconv_open("UTF-8", bom == g_bomLE ? "UCS-2LE" : "UCS-2BE");
		if(cd == (iconv_t)(-1)) EXCEPT;


		if((icr = iconv(cd, NULL, NULL, (char **)(&outptr), &outlen)) == (size_t)(-1)) EXCEPT; nconv += icr;
		if((icr = iconv(cd, (char **)(&inptr), &inlen, (char **)(&outptr), &outlen)) == (size_t)(-1)) EXCEPT; nconv += icr;
		if((icr = iconv(cd, NULL, NULL, (char **)(&outptr), &outlen)) == (size_t)(-1)) EXCEPT; nconv += icr;

		if(nconv)
			fprintf(stderr,"Iconv: warning - %d non-reversible conversions.",nconv);

		g_htmlconvlen = fin->fsize - 4 - outlen;

		//now search for UTF-16
		i = 0;
		while(i+7 < g_htmlconvlen && g_htmlconvptr[i]!= 0xd) {
			if( *(uint32_t*)(g_htmlconvptr+i)==0x46545522 && *(uint32_t*)(g_htmlconvptr+i+4)==0x2236312d ) {
				//found utf-16
				g_htmlconvptr[i+5] = '8';
				memmove(g_htmlconvptr+i+6, g_htmlconvptr+i+7,g_htmlconvlen-i-6);
//				g_htmlconvptr[i+6] = '8';
//				memmove(g_htmlconvptr+1, g_htmlconvptr,i+5);
				g_htmlconvlen--;
//				g_htmlconvoff--;
				break;
			}
			i++;
		}
//		memcpy(g_htmlconvptr, outptr, g_htmlconvlen);
	} else {
		g_htmlconvlen = fin->fsize - 2;
		memcpy(g_htmlconvptr, blobptr + 2, g_htmlconvlen);
	}

	ret = 0;
EXCEPT_LABEL;
	if(cd != (iconv_t)(-1))
		iconv_close(cd);
	if(blobptr)
		free(blobptr);

	return ret;
}

