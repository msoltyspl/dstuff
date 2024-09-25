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
#include "pooler.h"
#include "hashptr.h"
#include "textconv.h"
#include "xmlblob.h"
#include "xmldrivers.h"
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

//imports
//extern POOLER *g_pool_dynamic;

//exports
XML_NODE *g_xb_root = NULL;
STR_NODE *g_xb_strptr = NULL;
int g_xb_indent = 0, g_xb_strcnt = 0;

//locals
static HTAB *gs_htab = NULL; //hastab helper used in xml tree building, string offsets -> indexes
static uint8_t *gs_GPS_blobguard = NULL; //GetPackedSize blobguard, change it later

#if 0
static off_t Read(const int desc, uint8_t * const buffer, const off_t siz)
{
        off_t total=0, check;

	while(total < siz) {
		errno = 0;
                check = _TFR(read(desc, buffer+total, siz-total));
		if(check < 0)
			return  -1;
		if(check == 0)
			return total;
		total+=check;
	}
	return total;
}

static int CloseFile(const int desc)
{
	int ret = 0;
        if(desc >= 0) {
                ret = _TFR(close(desc));
	}
	return ret;
}
#endif

//uses gs_GPS_blobguard
static uintptr_t GetPackedSize(uint8_t **ptr)
{
	uintptr_t siz = 0, tmpsiz;
	int shift = 0;

	assert(gs_GPS_blobguard != NULL);

	do {
		tmpsiz = *((*ptr)++);
		siz += (tmpsiz & 0x7F)<<shift;
		shift += 7;
	} while ((tmpsiz & 0x80) && *ptr < gs_GPS_blobguard);

	if( tmpsiz & 0x80 )
		return UINTPTR_MAX;

	return siz;
}

static int XML_helper(uint8_t **ptr, int *dst)
{
	uintptr_t idx, dat;

	if((dat = GetPackedSize(ptr)) == UINTPTR_MAX)
		return -1;
	if((idx = HashTest(gs_htab, dat)) == UINTPTR_MAX)
		return -1;
	*dst = (int)idx;

	return 0;
}

static XML_NODE *XML_attach(uint8_t **ptr, int lvl)
{
	int cnt, i, flag;

	XML_NODE *nodeptr, *ret = NULL;

	if(g_xb_indent < lvl)
		g_xb_indent = lvl;

	if(!(nodeptr = (XML_NODE *)PoolerAssign(g_pool_dynamic, sizeof(XML_NODE)))) EXCEPT;

	//name
	if(XML_helper(ptr,&(nodeptr->name))) EXCEPT;

	//flag
	if((flag = (int)GetPackedSize(ptr)) == (int)UINTPTR_MAX) EXCEPT;
	if(flag & (~7u)) EXCEPT;

	//val
	if(flag & 1) {
		if(XML_helper(ptr,&(nodeptr->val))) EXCEPT;
	}
	//attrs
	if(flag & 2) {
		if((cnt = (int)GetPackedSize(ptr)) == (int)UINTPTR_MAX) EXCEPT;
		if(!(nodeptr->attr = (XML_PAIR *)PoolerAssign(g_pool_dynamic, cnt*sizeof(XML_PAIR)))) EXCEPT;
		nodeptr->nattr = cnt;
		for(i=0;i<cnt;i++) {
			if(XML_helper(ptr,&(nodeptr->attr[i].key))) EXCEPT;
			if(XML_helper(ptr,&(nodeptr->attr[i].val))) EXCEPT;
		}
	} else
		nodeptr->nattr = 0;
	//subnodes
	if(flag & 4) {
		if((cnt = (int)GetPackedSize(ptr)) == (int)UINTPTR_MAX) EXCEPT;
		if(!(nodeptr->subs = (XML_NODE **)PoolerAssign(g_pool_dynamic, cnt*sizeof(XML_NODE)))) EXCEPT;
		nodeptr->nsubs = cnt;
		for(i=0;i<cnt;i++) {
			if(!(nodeptr->subs[i] = XML_attach(ptr,lvl+1))) EXCEPT;
		}
	} else
		nodeptr->nsubs = 0;


	ret = nodeptr;
EXCEPT_LABEL;

	return ret;
}

/*
	Builds string table from blob's dictionary.
	Sets hashtable used by xml_helper, using offsets as keys, and string index as values.
*/

#define TIBCNT 32768
static int STRbuild(uint8_t *strptr, uint8_t *xmlptr,int bmode)
{
	int ret = -1, off8, i, utf8siz, cnt = 0;//, xml_flag = (g_curr_driver == XML);
	uint8_t *ucs2ptr, *utf8ptr, addbackslash_buf[TIBCNT*2];

	ucs2ptr = strptr;

	//prescan, do sanity checks, count number of strings
	while(ucs2ptr < xmlptr) {
		off8 = 0;
		while( (ucs2ptr+off8+1 < xmlptr) && *(uint16_t *)(ucs2ptr+off8) )
			off8 += 2;
		cnt++;
		ucs2ptr += off8 + 2;
	}
	if(ucs2ptr != xmlptr) {
		fputs("STRBuild: Misalignment of XML blob during sanity check.\n",stderr);
		printf("%u %u\n",(uintptr_t)ucs2ptr, (uintptr_t)xmlptr);
		EXCEPT;
		
	}
	printf("Found %d strings.\n",cnt);

	if(textconv_init(TIBCNT)) {
		fputs("STRBuild: Cannot allocate 32KB, o_O.\n",stderr);
		EXCEPT;
	}

	if(!(g_xb_strptr = (STR_NODE *)PoolerAssign(g_pool_dynamic, cnt*sizeof(STR_NODE))))
		EXCEPT;
	if(!(gs_htab = HashInit(cnt*3)))
		EXCEPT;

	ucs2ptr = strptr;
	g_xb_strcnt = cnt;

	for(i = 0; i < cnt ; i++) {
		off8 = 0;
		while( (ucs2ptr+off8+1 < xmlptr) && *(uint16_t *)(ucs2ptr+off8) )
			off8 += 2;
		if(textconv_conv("UTF-8", &utf8ptr, &utf8siz, "UCS-2LE", ucs2ptr, off8,0,1)<0) {
			fputs("STRBuild: Iconv error, cannot continue.\n",stderr);
			EXCEPT;
		}

		if((utf8siz = utf8add_bs(addbackslash_buf, utf8ptr,TIBCNT*2,utf8siz, bmode))<0) {
			fputs("STRBuild: virtually impossible to happen o_O.\n",stderr);
			EXCEPT;
		}

		if(!(g_xb_strptr[i].str = (char*)PoolerAssign(g_pool_dynamic, utf8siz+1))) {
			EXCEPT;
		}
		memcpy(g_xb_strptr[i].str, addbackslash_buf, utf8siz);
		g_xb_strptr[i].str[utf8siz] = 0; //throw 0 at the end for sake of sql driver output log messages
//		memcpy(g_xb_strptr[i].str, utf8ptr, utf8siz);
		g_xb_strptr[i].siz = utf8siz;
		g_xb_strptr[i].mark = 0;
#ifndef NDEBUG
		if(utf8siz > 2000) {
			FILE *gs_fout = fopen("dupa","ab+");
			fwrite(addbackslash_buf,utf8siz,1,gs_fout);
//			fwrite(utf8ptr,utf8siz,1,gs_fout);
			fwrite("\n\n\n",3,1,gs_fout);
			fclose(gs_fout);
		}
#endif

		if(HashSet(gs_htab, (ucs2ptr - strptr)>>1, i)) {
			fputs("STRBuild: Hashtable insert failed.\n",stderr);
			EXCEPT;
		}

		ucs2ptr += off8 + 2;
	}
//	HashDump(gs_htab);

	ret = 0;
EXCEPT_LABEL;
	textconv_destroy();
	return ret;
}


int ProcessXMLBlob(BFdesc *fin, int bmode)
{
	int ret = -1;

	uint8_t *blobptr = NULL;
	uint8_t *blobstrptr = NULL;
	uint8_t *blobxmlptr = NULL;
	uint8_t *tmpptr;
	uintptr_t tmpoff;

	if(fin->fsize < 1)
		return 0; //nothing to do, just close the file

	if(!(blobptr = (uint8_t*)malloc(fin->fsize)))
		EXCEPT;

	blobptr[0] = 0x80;

        if((bfread(fin, (char *)(blobptr+1), fin->fsize - 1)) != fin->fsize - 1)
		EXCEPT;

	
//	if(!(blobptr = (uint8_t*)malloc(fsizeTL)))
//		EXCEPT;

//        if((bfread(fin, (char *)blobptr, fsizeTL)) != fsizeTL)
//		EXCEPT;

	//hokay, blob read

	gs_GPS_blobguard = blobptr + fin->fsize;
	tmpptr = blobptr + 1;
	tmpoff = GetPackedSize(&tmpptr);

	if(tmpoff == UINTPTR_MAX)
		EXCEPT;

	blobstrptr = tmpptr;
	blobxmlptr = tmpptr + tmpoff;

	printf("File size (including xmlblob tag): %u\n", (uintptr_t)fin->fsize);
	printf("Offset to str blob: %u\n", blobstrptr - blobptr);
	printf("Offset to xml blob: %u\n", blobxmlptr - blobptr);


	//first rebuild strings table + convert to utf8

	STRbuild(blobstrptr, blobxmlptr, bmode);

	//now restructure blob

	tmpptr = blobxmlptr;
	if(!(g_xb_root = XML_attach(&tmpptr,0))) {
		fputs("Root XML_attach failed.\n",stderr);
		EXCEPT;
	}

	ret = 0;
EXCEPT_LABEL;
	if(blobptr)
		free(blobptr);
	HashDestroy(gs_htab);

	return ret;


}














#if 0
int ProcessBlob(const char* blobname, int bmode)
{
	struct stat statbuf;
//	off_t blobsiz;
	int blobdsc = -1;
	int ret = -1;
	uint8_t *blobptr = NULL;
	uint8_t *blobstrptr = NULL;
	uint8_t *blobxmlptr = NULL;
	uint8_t *tmpptr;
	uintptr_t tmpoff;
	
	if((ret = stat(blobname,&statbuf))<0 || !S_ISREG(statbuf.st_mode))
		EXCEPT;

//	blobsiz = statbuf.st_size;

	if(!(blobptr = (uint8_t*)malloc(statbuf.st_size)))
		EXCEPT;
        if((blobdsc = _TFR(open(blobname, O_RDONLY | O_BINARY)))<0)
		EXCEPT;
        if((Read(blobdsc, blobptr, statbuf.st_size))<0)
		EXCEPT;

	//hokay, blob read, output generic info

	if(blobptr[0] != 0x80) {
		fputs("File tag invalid, aborting.\n",stderr);
		EXCEPT;
	}

	gs_GPS_blobguard = blobptr + statbuf.st_size;
	tmpptr = blobptr + 1;
	tmpoff = GetPackedSize(&tmpptr);

	blobstrptr = tmpptr;
	blobxmlptr = tmpptr + tmpoff;

	puts("File tag valid: 0x80");
	printf("File size: %u\n", (uintptr_t)statbuf.st_size);
	printf("Offset to str blob: %u\n", blobstrptr - blobptr);
	printf("Offset to xml blob: %u\n", blobxmlptr - blobptr);


	//first rebuild strings table + convert to utf8

	STRbuild(blobstrptr,blobxmlptr,bmode);

	//now restructure blob

	tmpptr = blobxmlptr;
	if(!(g_xb_root = XML_attach(&tmpptr,0))) {
		fputs("Root XML_attach failed.\n",stderr);
		EXCEPT;
	}

	ret = 0;
EXCEPT_LABEL;
	if(blobptr)
		free(blobptr);
	CloseFile(blobdsc);
	HashDestroy(gs_htab);

	return ret;


}
#endif
