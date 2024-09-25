#include <string.h>
#include <stdio.h>
#include "cfgdrivers.h"
#include "cfgblob.h"
#include "bfile.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

int CFGDriverOut(BFdesc *fout)
{
	int ret = -1;

	//sanity check

	if(!g_cfgconvptr || !g_cfgconvlen) {
		fputs("CFGDriver: decryption invalid ? o_O.\n",stderr);
		EXCEPT;
	}

	if(bfwrite(fout,(char*)g_cfgconvptr,g_cfgconvlen)<0) {
		fputs("CFGDriver: cannot write.\n",stderr);
		EXCEPT;
	}

	ret = 0;

EXCEPT_LABEL;

	return ret;
}


