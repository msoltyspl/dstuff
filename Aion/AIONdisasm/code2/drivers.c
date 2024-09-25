#include <string.h>
#include <stdio.h>
#include "drivers.h"
#include "xmlblob.h"
#include "bfile.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:
#define MAXINDENT 256

//drvtype g_curr_driver = TEXT;

static void (*gs_vout)(XML_NODE*) = NULL;

//static FILE *gs_fout = NULL;
static BFdesc *gs_bfout = NULL;

static char gs_tt[MAXINDENT];
static const char g_xmltag[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";

static void DriverXMLoutR(XML_NODE *ptr, int tidx)
{
	int i;

	//name

	bfwrite(gs_bfout, "\n", 1);
	bfwrite(gs_bfout, gs_tt, tidx);
	bfwrite(gs_bfout, "<", 1);
	bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);

	//attrs

	if(ptr->nattr) {
		for(i=0;i<ptr->nattr;i++) {
			bfwrite(gs_bfout, " ", 1);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].key].str, g_xb_strptr[ptr->attr[i].key].siz);
			bfwrite(gs_bfout, "=\"", 2);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].val].str, g_xb_strptr[ptr->attr[i].val].siz);
			bfwrite(gs_bfout, "\"", 1);
		}
	}
	bfwrite(gs_bfout, ">", 1);

	//val

	if(g_xb_strptr[ptr->val].siz) {
		bfwrite(gs_bfout, "\n", 1);
		bfwrite(gs_bfout, gs_tt, tidx+1);
		bfwrite(gs_bfout, g_xb_strptr[ptr->val].str, g_xb_strptr[ptr->val].siz);
	}

	//children

	if(ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
//			fputc('\n',gs_fout);
			DriverXMLoutR(ptr->subs[i],tidx+1);
		}
	}
	
	//close name
	bfwrite(gs_bfout, "\n", 1);
	bfwrite(gs_bfout, gs_tt, tidx);
	bfwrite(gs_bfout, "</", 2);
	bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);
	bfwrite(gs_bfout, ">", 1);
}

static void DriverXMLout(XML_NODE *root)
{
	bfwrite(gs_bfout, g_xmltag, strlen(g_xmltag));
	DriverXMLoutR(root,0);
	bfwrite(gs_bfout, "\n", 1);
}

static void DriverTEXToutR(XML_NODE *ptr, int tidx)
{
	int i;

	//name

//	fputc('\n',gs_fout);
	bfwrite(gs_bfout, gs_tt, tidx);           
	bfwrite(gs_bfout, "N: ", 3);
	bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);

	//val

	if(g_xb_strptr[ptr->val].siz) {
		bfwrite(gs_bfout, "\n", 1);
		bfwrite(gs_bfout, gs_tt, tidx);
		bfwrite(gs_bfout, "+- v: ", 6);
		bfwrite(gs_bfout, g_xb_strptr[ptr->val].str, g_xb_strptr[ptr->val].siz);
	}

	//attrs

	if(ptr->nattr) {
		bfwrite(gs_bfout, "\n", 1);
		bfwrite(gs_bfout, gs_tt, tidx);
		bfwrite(gs_bfout, "+- a:", 6);
		for(i=0;i<ptr->nattr;i++) {
			bfwrite(gs_bfout, " ", 1);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].key].str, g_xb_strptr[ptr->attr[i].key].siz);
			bfwrite(gs_bfout, "=\"", 2);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].val].str, g_xb_strptr[ptr->attr[i].val].siz);
			bfwrite(gs_bfout, "\"", 1);
		}
	}

	//children

	if(ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
			bfwrite(gs_bfout, "\n", 1);
			DriverTEXToutR(ptr->subs[i],tidx+1);
		}
	}
}



static void DriverTEXTout(XML_NODE *root)
{
	DriverTEXToutR(root,0);
	bfwrite(gs_bfout, "\n", 1);
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
//	g_curr_driver = idx;

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
		fputs("XML tree not prepared ? o_O.\n",stderr);
		EXCEPT;
	}

	if( !(gs_bfout = bfopen(w,outname,0))) {
		fprintf(stderr,"Cannot open output file '%s'.\n",outname);
		EXCEPT;
	}

	if(g_xb_indent  >= MAXINDENT) {
		fputs("Indentation too big.\n",stderr);
		EXCEPT;
	}

	gs_vout(g_xb_root);

	ret = 0;

EXCEPT_LABEL;

	if(gs_bfout) {
		bfclose(gs_bfout);
	}

	return ret;
}


