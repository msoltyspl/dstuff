#include <assert.h>
#include <stdint.h>
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
#include "drivers.h"
#include "version.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

static char *gs_input_file = NULL;
static char *gs_output_file = NULL;

POOLER *g_pool_dynamic = NULL;
int g_bslashmode = 1;

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
	puts("AIONdisasm [options] input_file output_file");
	puts("-d <val>    : select output driver:");
	puts("              0 - TEXT (default)");
	puts("              1 - XML");
	puts("-b <val>    : select backspacing:");
	puts("              0 - none");
	puts("              1 - backslash (default)");
	puts("              2 - XML (enforced if output is XML)");
	puts("-h          : this help\n");
}

int Parse(int argc, char **argv, int *drv, int* bmode)
{
	int c;

	opterr = 0;
	*drv = 0;

	while ((c = getopt (argc, argv, ":d:b:h")) != -1) {
		switch(c) {
			case 'd':
				*drv = *optarg - 0x30;
				break;
			case 'b':
				*bmode = *bmode - 0x30;
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
	if(*bmode<0 || *bmode>2) {
		fputs("Invalid backslash moder selected.\n",stderr);
		return -1;
	}
	if(*drv<0 || *drv>1) {
		fputs("Invalid driver selected.\n",stderr);
		return -1;
	} else if(*drv == 1) {
		fputs("Info: backslash mode forced to 2.\n",stderr);
		*bmode = 2;
	}
	if(optind+1 < argc) {
		gs_input_file = argv[optind];
		gs_output_file = argv[optind+1];
		if(!strcmp(gs_input_file,gs_output_file)) {
			fputs("Please use different file names.\n",stderr);
			return -1;
		}
	} else {
		fputs("Missing output and/or input filename.\n",stderr);
		Help();
		return -1;
	}	
        return 0;
}

int main(int argc, char** argv)
{
	int ret = -1, drv, bmode = 1;

	puts("\nAIONdisasm "DATT_VERSION" by "DATT_AUTHOR".\n");
	if(Parse(argc, argv, &drv, &bmode)<0)
		EXCEPT;

	g_pool_dynamic = PoolerInit("Dynamic");
	if(!(g_pool_dynamic))
		EXCEPT;

	if(DriverInit(drv)<0)
		EXCEPT;

//	PoolerFree(g_pool_dynamic);
//	if(textconv_init(g_pool_static,STR_HARD)<0)
//		EXCEPT;
//	textconv_summary();

	if(ProcessBlob(gs_input_file, bmode)<0) {
		fputs("AIONdisasm: ProcessBlob failed.\n",stderr);
		EXCEPT;
	}

	DriverOut(gs_output_file);

	ret = 0;

EXCEPT_LABEL;
	PoolerDestroy(g_pool_dynamic);

	return ret;
}
