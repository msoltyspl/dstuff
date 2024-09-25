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
#include "version.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

static char *gs_input_file = NULL;
static char *gs_output_file = NULL;

POOLER *g_pool_static = NULL;
POOLER *g_pool_dynamic = NULL;
int g_output_driver = 0;

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
	puts("AIONenema [options] input_file output_file\n");
	puts("-d <val>    : select output driver\n");
}

int Parse(int argc, char **argv)
{
	int c;

	opterr = 0;

	while ((c = getopt (argc, argv, ":d:")) != -1) {
		switch(c) {
			case 'd':
				g_output_driver = (*optarg & 0x7f) - 0x30;
				break;
			case '?':
				puts("Unknown option.\n");
				Help();
				return -1;
			case ':':
				puts("Missing required argument.\n");
				Help();
				return -1;
			default:
				puts("Getopt failure ? WTF !\n");
				return -255;
		}
	}
	if(optind+1 < argc) {
		gs_input_file = argv[optind];
		gs_output_file = argv[optind+1];
		if(!strcmp(gs_input_file,gs_output_file)) {
			puts("Please use different file names.\n");
			return -1;
		}
	} else {
		puts("Missing output and/or input filename.\n");
		Help();
		return -1;
	}	
        return 0;
}

int main(int argc, char** argv)
{
	int ret = -1;

	puts("\nAION enema "DATT_VERSION" by "DATT_AUTHOR".\n");

//	g_pool_static=PoolerInit("Static");
	g_pool_dynamic=PoolerInit("Dynamic");
//	if(!(g_pool_static && g_pool_dynamic))
	if(!(g_pool_dynamic))
		EXCEPT;

	if(Parse(argc, argv)<0)
		EXCEPT;

//	PoolerFree(g_pool_dynamic);
//	if(textconv_init(g_pool_static,STR_HARD)<0)
//		EXCEPT;
//	textconv_summary();

	ProcessBlob(gs_input_file, gs_output_file, g_pool_dynamic, NULL);

	ret = 0;

EXCEPT_LABEL;
//	PoolerDestroy(g_pool_static);
	PoolerDestroy(g_pool_dynamic);

	return ret;
}
