#include <string.h>
#include <stdio.h>
#include "htmldrivers.h"
#include "htmlblob.h"
#include "bfile.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

int HTMLDriverOut(BFdesc *fout)
{
	int ret = -1;

	//sanity check

	if(!g_htmlconvptr || !g_htmlconvlen) {
		fputs("HTMLDriver: decryption invalid ? o_O.\n",stderr);
		EXCEPT;
	}

	if(bfwrite(fout,(char*)g_htmlconvptr,g_htmlconvlen)<0) {
		fputs("HTMLDriver: cannot write.\n",stderr);
		EXCEPT;
	}

	ret = 0;

EXCEPT_LABEL;

	return ret;
}


