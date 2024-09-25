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
extern POOLER *g_pool_dynamic;
extern int g_output_driver;

//exports
XML_NODE *g_xmltree = NULL;
STR_NODE *g_strptr = NULL;

//locals
static HTAB *gs_htab = NULL;
static uint8_t *gs_blobptr = NULL;
static uint8_t *gs_blobstrptr = NULL;
static uint8_t *gs_blobxmlptr = NULL;
static uint8_t *gs_GPS_blobguard = NULL;

static off_t Read(const int desc, uint8_t * const buffer, const off_t siz)
{
        off_t total=0, check;

	while(total < siz) {
		errno = 0;
                check = _TFR(read(desc, buffer+total, siz-total));
		if(check < 0)
			return  -1;
//		if(check == 0)
//			return total;
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

static XML_NODE *XML_attach(uint8_t **ptr)
{
	int cnt, i, flag;

	XML_NODE *nodeptr, *ret = NULL;

	if(!(nodeptr = (XML_NODE *)PoolerAssign(g_pool_dynamic, sizeof(XML_NODE)))) EXCEPT;

	//name
	if(XML_helper(ptr,&(nodeptr->name))) EXCEPT;

	//flag
	if((flag = (int)GetPackedSize(ptr)) == UINTPTR_MAX) EXCEPT;
	if(flag & (~7u)) EXCEPT;

	//val
	if(flag & 1) {
		if(XML_helper(ptr,&(nodeptr->val))) EXCEPT;
	}
	//attrs
	if(flag & 2) {
		if((cnt = (int)GetPackedSize(ptr)) == UINTPTR_MAX) EXCEPT;
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
		if((cnt = (int)GetPackedSize(ptr)) == UINTPTR_MAX) EXCEPT;
		if(!(nodeptr->subs = (XML_NODE **)PoolerAssign(g_pool_dynamic, cnt*sizeof(XML_NODE)))) EXCEPT;
		nodeptr->nsubs = cnt;
		for(i=0;i<cnt;i++) {
			if(!(nodeptr->subs[i] = XML_attach(ptr))) EXCEPT;
		}
	} else
		nodeptr->nsubs = 0;


	ret = nodeptr;
EXCEPT_LABEL;

	return ret;
}

//used by test print driver and processblob
FILE *gs_fout = NULL;

static void XML_print(XML_NODE *ptr, int tidx)
{
	static const char * const tt = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int i;

	//name

//	fwrite(tabtab[tidx].str, tabtab[tidx].siz, 1, gs_fout);
//	fputc('\n',gs_fout);
	fwrite(tt, 1, tidx, gs_fout);
	fwrite("N: ", 3, 1, gs_fout);
	fwrite(g_strptr[ptr->name].str, g_strptr[ptr->name].siz, 1, gs_fout);

	//val

	if(g_strptr[ptr->val].siz) {
		fputc('\n',gs_fout);
//	fwrite(tabtab[tidx].str, tabtab[tidx].siz, 1, gs_fout);
		fwrite(tt, 1, tidx, gs_fout);
		fwrite("+- v: ", 6, 1, gs_fout);
		fwrite(g_strptr[ptr->val].str, g_strptr[ptr->val].siz, 1, gs_fout);
	}

	//attrs

	if(ptr->nattr) {
		fputc('\n',gs_fout);
//	fwrite(tabtab[tidx].str, tabtab[tidx].siz, 1, gs_fout);
		fwrite(tt, 1, tidx, gs_fout);
		fwrite("+- a:", 6, 1, gs_fout);
		for(i=0;i<ptr->nattr;i++) {
			fputc(' ',gs_fout);
			fwrite(g_strptr[ptr->attr[i].key].str, g_strptr[ptr->attr[i].key].siz, 1, gs_fout);
			fwrite("=\"", 2, 1, gs_fout);
			fwrite(g_strptr[ptr->attr[i].val].str, g_strptr[ptr->attr[i].val].siz, 1, gs_fout);
			fputc('"',gs_fout);
		}
	}

	//children

	if(ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
			fputc('\n',gs_fout);
			XML_print(ptr->subs[i],tidx+1);
		}
	}
/*
	//name
	fprintf(gs_fout,"%*sN: %s\n",tab,"",g_strptr[ptr->name]);
	//val
	fprintf(gs_fout,"%*s+- v: %s\n",tab,"",g_strptr[ptr->val]);
	for(i=0;i<ptr->nattr;i++) {
		fprintf(gs_fout,"%*s+ a: %s=\"%s\"\n",tab,"",g_strptr[ptr->attr[i].key],g_strptr[ptr->attr[i].val]);
	}
	for(i=0;i<ptr->nsubs;i++) {
		XML_print(ptr->subs[i],tab+1);
	}
*/
//	fputc('\n',gs_fout);
}







/*
static void PutPackedSize(uint8_t **ptr, uintptr_t siz)
{
	int hpacks;

	do {
		hpacks = siz & 127;
		siz = siz >> 7;
		if(siz) {
			hpacks |= 0x80;
		}
		*(*ptr)++ = hpacks;
	} while(siz);
}
*/

static int STRbuild(void)
{
	int ret = -1, off8, i, utf8siz, cnt = 0;
	uint8_t *ucs2ptr, *utf8ptr;

	ucs2ptr = gs_blobstrptr;

	//prescan, do sanity checks, count number of strings
	while(ucs2ptr < gs_blobxmlptr) {
		off8 = 0;
		while( (ucs2ptr+off8+1 < gs_blobxmlptr) && *(uint16_t *)(ucs2ptr+off8) )
			off8 += 2;
		cnt++;
		ucs2ptr += off8 + 2;
	}
	if(ucs2ptr != gs_blobxmlptr) {
		fputs("STRBuild: Misalignment of XML blob during sanity check.",stderr);
		printf("%u %u\n",(uintptr_t)ucs2ptr, (uintptr_t)gs_blobxmlptr);
		EXCEPT;
		
	}
	printf("Found %d strings.\n",cnt);

	if(textconv_init(32768)) {
		fputs("STRBuild: Cannot allocate 32KB, o_O",stderr);
		EXCEPT;
	}

	if(!(g_strptr = (STR_NODE *)PoolerAssign(g_pool_dynamic, cnt*sizeof(STR_NODE))))
		EXCEPT;
	if(!(gs_htab = HashInit(cnt*3)))
		EXCEPT;

	ucs2ptr = gs_blobstrptr;

	for(i = 0; i < cnt ; i++) {
		off8 = 0;
		while( (ucs2ptr+off8+1 < gs_blobxmlptr) && *(uint16_t *)(ucs2ptr+off8) )
			off8 += 2;
		if(textconv_conv("UTF-8", &utf8ptr, &utf8siz, "UCS-2LE", ucs2ptr, off8,0,1)<0) {
			fputs("STRBuild: Iconv error, cannot continue.",stderr);
			EXCEPT;
		}

//		if(!(g_strptr[i] = (char*)PoolerAssign(g_pool_dynamic, utf8siz+1))) {
//			EXCEPT;
//		}
//		if(!(g_strptr[i] = (STR_NODE*)PoolerAssign(g_pool_dynamic, sizeof(STR_NODE)))) {
//			EXCEPT;
//		}
		if(!(g_strptr[i].str = (char*)PoolerAssign(g_pool_dynamic, utf8siz))) {
			EXCEPT;
		}
		memcpy(g_strptr[i].str, utf8ptr,utf8siz);
		g_strptr[i].siz = utf8siz;

		if(HashSet(gs_htab, (ucs2ptr - gs_blobstrptr)>>1, i)) {
			fputs("STRBuild: Hashtable insert failed.",stderr);
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



int ProcessBlob(const char* blobname, const char* outname)
{
	struct stat statbuf;
//	off_t blobsiz;
	int blobdsc = -1;
	int ret = -1;
	uint8_t *tmpptr;
	uintptr_t tmpoff;
	XML_NODE *root = NULL;
	
	if((ret = stat(blobname,&statbuf))<0 || !S_ISREG(statbuf.st_mode))
		EXCEPT;

//	blobsiz = statbuf.st_size;

	if(!(gs_blobptr = (uint8_t*)malloc(statbuf.st_size)))
		EXCEPT;
        if((blobdsc = _TFR(open(blobname, O_RDONLY | O_BINARY)))<0)
		EXCEPT;
        if((Read(blobdsc, gs_blobptr, statbuf.st_size))<0)
		EXCEPT;

	//hokay, blob read, output generic info

	if(gs_blobptr[0] != 0x80) {
		fputs("File tag invalid, aborting.",stderr);
		EXCEPT;
	}

	gs_GPS_blobguard = gs_blobptr + statbuf.st_size;
	tmpptr = gs_blobptr + 1;
	tmpoff = GetPackedSize(&tmpptr);

	gs_blobstrptr = tmpptr;
	gs_blobxmlptr = tmpptr + tmpoff;

	puts("File tag valid: 0x80");
	printf("File size: %u\n", (uintptr_t)statbuf.st_size);
	printf("Offset to str blob: %u\n", gs_blobstrptr - gs_blobptr);
	printf("Offset to xml blob: %u\n", gs_blobxmlptr - gs_blobptr);


	//first rebuild strings table + convert to utf8

	STRbuild();

	//now restructure blob

	tmpptr = gs_blobxmlptr;
	if(!(root = XML_attach(&tmpptr))) {
		fputs("Root XML_attach failed.",stderr);
		EXCEPT;
	}

	if(!(gs_fout = fopen(outname,"wb"))) {
		printf("Cannot open output file '%s'.\n",outname);
		EXCEPT;
	}

/*	//prepare tabulation index tab
	for(i=0;i<16;i++) {
		if(!(tabtab[i].str = (char *)PoolerAssign(g_pool_dynamic, i*8)))
			EXCEPT;
		memset(tabtab[i].str,' ',i*8);
		tabtab[i].siz = i*8;
	}
*/
	XML_print(root,0);
	fputc('\n',gs_fout);

	ret = 0;
EXCEPT_LABEL;
	if(gs_blobptr)
		free(gs_blobptr);
	if(gs_fout)
		fclose(gs_fout);
	CloseFile(blobdsc);
	HashDestroy(gs_htab);

	return ret;


}
