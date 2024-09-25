#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <math.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <string.h>
#include <malloc.h>

#include "pooler.h"
#include "hashptr.h"
#include "xmlblob.h"
#include "xmldrivers.h"
#include "htmlblob.h"
#include "htmldrivers.h"
#include "cfgblob.h"
#include "cfgdrivers.h"
#include "version.h"
#include "tags.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

static char *gs_nfin = NULL, *gs_nfout = NULL;

static BFdesc *gs_fin = NULL, *gs_fout = NULL;



//int g_bslashmode = 1;

/*
void FreeBuffers(int cnt, ...)
{
	int i;
	va_list ap;
	va_start(ap, cnt);
	void *ptr;

	for(i=0;i<cnt;i++) {
		ptr = va_arg(ap, void*);
		if(ptr)
			free(ptr);
	}
	va_end(ap);
}
*/

void Help(void)
{
	puts(
	"\nAIONdisasm [options] input_file output_file\n"
	"\nXML options: (ignored if xml tag not detected)\n"
	"-d <val>    : select output driver:\n"
	"              0 - TEXT\n"
	"              1 - XML (default)\n"
	"-b <val>    : select backspacing:\n"
	"              0 - none\n"
	"              1 - backslash\n"
	"              2 - XML (default, enforced if output is XML)\n"
	"\nHTML options: (ignored if html tag not detected)\n"
	"-r          : output UTF-16\n"
	"\nCFG options: (only if tag is not detected)\n"
	"-c          : assume file is .cfg\n"
	"\nother options:\n"
	"-h          : this help\n"
	);
}

int Parse(int argc, char **argv, int *drv, int* bmode, int* r, int *cfg)
{
	char *tailptr;
	int c;

	opterr = 0;
	*drv = 1;
	*bmode = 2;
	*r = 0;
	*cfg = 0;

	while ((c = getopt (argc, argv, ":d:b:hrc")) != -1) {
		switch(c) {
			case 'd':
				errno = 0; *drv = strtol(optarg,&tailptr,10);
				if(tailptr == optarg || errno || *tailptr != '\0') {
					fputs("Unparsable driver number.\n",stderr);
					Help();
					return -1;
				}
//				*drv = *optarg - 0x30;
				break;
			case 'b':
				errno = 0; *bmode = strtol(optarg,&tailptr,10);
				if(tailptr == optarg || errno || *tailptr != '\0') {
					fputs("Unparsable backslash mode number.\n",stderr);
					Help();
					return -1;
				}
//				*bmode = *optarg - 0x30;
				break;
			case 'r':
				*r = 1;
				break;
			case 'c':
				*cfg = 1;
				break;
			case 'h':
				Help();
				return -1;
			case '?':
				fputs("Unknown option.\n",stderr);
				Help();
				return -1;
			case ':':
				fputs("Missing required argument.\n",stderr);
				Help();
				return -1;
			default:
				fputs("Getopt failure ? WTF !\n",stderr);
				return -255;
		}
	}
	if(*drv == 1084)
		*drv = 2;
	if(optind+1 < argc) {
		gs_nfin = argv[optind];
		gs_nfout = argv[optind+1];
		if(!strcmp(gs_nfin,gs_nfout)) {
			fputs("Please use different file names.\n",stderr);
			return -1;
		}
	} else {
		fputs("Missing output and/or input filename.\n",stderr);
		Help();
		return -1;
	}	
//	if(*drv != 1084 && (*drv<0 || *drv>1)) {
	if((*drv<0 || *drv>2)) {
		fputs("Invalid driver selected.\n",stderr);
		return -1;
//	} else if( (*drv == 1 || *drv == 1084) && *bmode != 2) {
	} else if( (*drv == 1 || *drv == 2) && *bmode != 2) {
		fputs("Info: backslash mode forced to 2.\n",stderr);
		*bmode = 2;
	}
	if(*bmode<0 || *bmode>2) {
		fputs("Invalid backslash mode selected.\n",stderr);
		return -1;
	}
        return 0;
}

int ProcessBlob(int drv, int bmode, int conv, int cfg)
{
	int ret = -1;
	int tag = 0;

	if(bfread(gs_fin, (char*)&tag,1) != 1) {
		fputs("PB: Failed to read the tag.\n",stderr);
		EXCEPT;
	}

	switch(tag) {
		case 0x80:
			if(XMLDriverInit(drv)<0) {
				fputs("PB: XML driver init failed.\n",stderr);
				EXCEPT;
			}

			if(ProcessXMLBlob(gs_fin, bmode)<0) {
				fputs("PB: ProcessXMLBlob failed.\n",stderr);
				EXCEPT;
			}

			if(XMLDriverOut(gs_fout)<0) {
				fputs("PB: XML output driver failed.\n",stderr);
				EXCEPT;
			}
			XMLDriverDestroy();
			break;

		case 0x81:
			if(ProcessHTMLBlob(gs_fin,conv)<0) {
				fputs("PB: ProcessHTMLBlob failed.\n",stderr);
				EXCEPT;
			}
			if(HTMLDriverOut(gs_fout)<0) {
				fputs("PB: HTML output driver failed.\n",stderr);
				EXCEPT;
			}
			break;

		default:
			if(!cfg) {
				fputs("PB: unknown tag.\n",stderr);
				EXCEPT;
			}
			if(ProcessCFGBlob(gs_fin, (uint32_t)tag)<0) {
				fputs("PB: ProcessCFGBlob failed.\n",stderr);
				EXCEPT;
			}
			if(CFGDriverOut(gs_fout)<0) {
				fputs("PB: CFG output driver failed.\n",stderr);
				EXCEPT;
			}
	}


	ret = 0;
EXCEPT_LABEL;

	return ret;
}


int main(int argc, char** argv)
{
	int ret = -1, drv, bmode, conv, cfg;

	puts("\nAIONdisasm "DATT_VERSION" by "DATT_AUTHOR".\n");
	puts("Special thanks to Fyyre for assistance with\nretarded GG/Themida/protectors/etc.\n");
	if(Parse(argc, argv, &drv, &bmode, &conv, &cfg)<0)
		EXCEPT;

	g_pool_dynamic = PoolerInit("Pooler");

	if(!(g_pool_dynamic))
		EXCEPT;

	if(!(gs_fin = bfopen(r, gs_nfin, 0))) {
		fputs("main: Cannot open input file.\n",stderr);
		EXCEPT;
	}

	if(!(gs_fout = bfopen(w, gs_nfout, 0))) {
		fputs("main: Cannot open output file.\n",stderr);
		EXCEPT;
	}

	if(ProcessBlob(drv, bmode, conv, cfg)<0) {
		fputs("main: ProcessBlob failed.\n",stderr);
		EXCEPT;
	}
//	printf(" ------- %d\n",fsizeTL);


//	PoolerFree(g_pool_dynamic);
//	if(textconv_init(g_pool_static,STR_HARD)<0)
//		EXCEPT;
//	textconv_summary();

	ret = 0;

EXCEPT_LABEL;
	PoolerDestroy(g_pool_dynamic);
	if(gs_fin)
		bfclose(gs_fin);
	if(gs_fout)
		bfclose(gs_fout);

	return ret;
}
