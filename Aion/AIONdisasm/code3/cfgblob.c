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

#include "pooler.h"
#include "cfgblob.h"
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

static const char gs_bfr_err[]="ProcessCFGBlob: bfread failed.\n";
static const char gs_mal_err[]="ProcessCFGBlob: malloc failed.\n";
static const char gs_pool_err[]="ProcessCFGBlob: pooler failed.\n";

//imports
//extern POOLER *g_pool_dynamic;

//exports
uint8_t *g_cfgconvptr = NULL;
int g_cfgconvlen = 0;//, g_htmlconvoff = 0;

int ProcessCFGBlob(BFdesc *fin, uint32_t chr)
{
	int ret = -1, i;

	uint8_t *blobptr = NULL;

	if(!fin->fsize)
		return 0; //nothing to do, just close the file

	if(!(blobptr = (uint8_t*)malloc(fin->fsize))) {
		fputs(gs_mal_err,stderr);
		EXCEPT;
	}
	if(!(g_cfgconvptr = (uint8_t*)PoolerAssign(g_pool_dynamic, fin->fsize))) {
		fputs(gs_pool_err,stderr);
		EXCEPT;
	}

	blobptr[0] = chr;

        if((bfread(fin, (char *)(blobptr+1), fin->fsize - 1)) != fin->fsize - 1) {
		fputs(gs_bfr_err,stderr);
		EXCEPT;
	}

	printf("File size: %u\n", (uintptr_t)(fin->fsize));


	for(i=0;i<fin->fsize;i++) {
		if(blobptr[i] & 0x80)
			blobptr[i] ^= 0xFF;
	}

	g_cfgconvlen = fin->fsize;
	memcpy(g_cfgconvptr, blobptr , g_cfgconvlen);

	ret = 0;
EXCEPT_LABEL;
	if(blobptr)
		free(blobptr);

	return ret;
}

