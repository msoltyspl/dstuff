#include <stdio.h>
#include "drivers.h"
#include "xmlblob.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:


static (*gs_vptr);

static FILE *gs_fout = NULL;


static void DriverXMLoutV(XML_NODE *ptr, int tidx)

static int DriverXMLout()
{
}





static void DriverTEXToutV(XML_NODE *ptr, int tidx)
{
	static const char * const tt = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int i;

	//name

//	fputc('\n',gs_fout);
	fwrite(tt, 1, tidx, gs_fout);
	fwrite("N: ", 3, 1, gs_fout);
	fwrite(g_strptr[ptr->name].str, g_strptr[ptr->name].siz, 1, gs_fout);

	//val

	if(g_strptr[ptr->val].siz) {
		fputc('\n',gs_fout);
		fwrite(tt, 1, tidx, gs_fout);
		fwrite("+- v: ", 6, 1, gs_fout);
		fwrite(g_strptr[ptr->val].str, g_strptr[ptr->val].siz, 1, gs_fout);
	}

	//attrs

	if(ptr->nattr) {
		fputc('\n',gs_fout);
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
}



static int DriverTEXTout()
{
}




void DriverInit(drvtype idx)
{
	
}




int DriverOut(const char* outname)
{
	int ret = -1;

	//sanity check

	if(!g_xmltree || !g_strptr)
		EXCEPT;

	if(!(gs_fout = fopen(outname,"wb"))) {
		printf("Cannot open output file '%s'.\n",outname);
		EXCEPT;
	}



	ret = 0;

EXCEPT_LABEL;

	if(gs_fout)
		fclose(gs_fout);

	return ret;
}








#endif
