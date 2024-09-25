#include <string.h>
#include <stdio.h>
#include "drivers.h"
#include "xmlblob.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:
#define MAXINDENT 256

drvtype g_curr_driver = TEXT;

static void (*gs_vout)(XML_NODE*) = NULL;

static FILE *gs_fout = NULL;
static char gs_tt[MAXINDENT];

static void DriverXMLoutR(XML_NODE *ptr, int tidx)
{
	int i;

	//name

	fputc('\n',gs_fout);
	fwrite(gs_tt, tidx, 1,  gs_fout);
	fputc('<',gs_fout);
	fwrite(g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz, 1, gs_fout);

	//attrs

	if(ptr->nattr) {
		for(i=0;i<ptr->nattr;i++) {
			fputc(' ',gs_fout);
			fwrite(g_xb_strptr[ptr->attr[i].key].str, g_xb_strptr[ptr->attr[i].key].siz, 1, gs_fout);
			fwrite("=\"", 2, 1, gs_fout);
			fwrite(g_xb_strptr[ptr->attr[i].val].str, g_xb_strptr[ptr->attr[i].val].siz, 1, gs_fout);
			fputc('"',gs_fout);
		}
	}
	fputc('>',gs_fout);

	//val

	if(g_xb_strptr[ptr->val].siz) {
		fputc('\n',gs_fout);
		fwrite(gs_tt, tidx+1, 1, gs_fout);
		fwrite(g_xb_strptr[ptr->val].str, g_xb_strptr[ptr->val].siz, 1, gs_fout);
	}

	//children

	if(ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
//			fputc('\n',gs_fout);
			DriverXMLoutR(ptr->subs[i],tidx+1);
		}
	}
	
	//close name
	fputc('\n',gs_fout);
	fwrite(gs_tt, tidx, 1,  gs_fout);
	fwrite("</", 2, 1, gs_fout);
	fwrite(g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz, 1, gs_fout);
	fputc('>',gs_fout);
}

static void DriverXMLout(XML_NODE *root)
{
	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>",gs_fout);
	DriverXMLoutR(root,0);
	fputc('\n',gs_fout);
}

static void DriverTEXToutR(XML_NODE *ptr, int tidx)
{
	int i;

	//name

//	fputc('\n',gs_fout);
	fwrite(gs_tt, tidx, 1,  gs_fout);
	fwrite("N: ", 3, 1, gs_fout);
	fwrite(g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz, 1, gs_fout);

	//val

	if(g_xb_strptr[ptr->val].siz) {
		fputc('\n',gs_fout);
		fwrite(gs_tt, tidx, 1, gs_fout);
		fwrite("+- v: ", 6, 1, gs_fout);
		fwrite(g_xb_strptr[ptr->val].str, g_xb_strptr[ptr->val].siz, 1, gs_fout);
	}

	//attrs

	if(ptr->nattr) {
		fputc('\n',gs_fout);
		fwrite(gs_tt, tidx, 1,  gs_fout);
		fwrite("+- a:", 6, 1, gs_fout);
		for(i=0;i<ptr->nattr;i++) {
			fputc(' ',gs_fout);
			fwrite(g_xb_strptr[ptr->attr[i].key].str, g_xb_strptr[ptr->attr[i].key].siz, 1, gs_fout);
			fwrite("=\"", 2, 1, gs_fout);
			fwrite(g_xb_strptr[ptr->attr[i].val].str, g_xb_strptr[ptr->attr[i].val].siz, 1, gs_fout);
			fputc('"',gs_fout);
		}
	}

	//children

	if(ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
			fputc('\n',gs_fout);
			DriverTEXToutR(ptr->subs[i],tidx+1);
		}
	}
}



static void DriverTEXTout(XML_NODE *root)
{
	DriverTEXToutR(root,0);
	fputc('\n',gs_fout);
}

int DriverInit(drvtype idx)
{
	int ret = -1;

	switch(idx) {
		case TEXT:
			gs_vout = &DriverTEXTout; break;
		case XML:
			gs_vout = &DriverXMLout; break;
		default:
			EXCEPT;
	}
	g_curr_driver = idx;

	memset(gs_tt,'\t',MAXINDENT);

	ret = 0;
EXCEPT_LABEL;
	return ret;
}

int DriverOut(const char* outname)
{
	int ret = -1;

	//sanity check

	if(!g_xb_root || !g_xb_strptr) {
		fputs("XML tree not prepared ? o_O.",stderr);
		EXCEPT;
	}

	if(!(gs_fout = fopen(outname,"wb"))) {
		fprintf(stderr,"Cannot open output file '%s'.\n",outname);
		EXCEPT;
	}

	if(g_xb_indent  >= MAXINDENT) {
		fputs("Indentation too big.",stderr);
		EXCEPT;
	}

	gs_vout(g_xb_root);

	ret = 0;

EXCEPT_LABEL;

	if(gs_fout)
		fclose(gs_fout);

	return ret;
}


