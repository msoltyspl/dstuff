#ifndef __htmlblob_h__
#define __htmlblob_h__

#include "bfile.h"

int ProcessHTMLBlob(BFdesc *fin, int r);

extern uint8_t *g_htmlconvptr;
extern int g_htmlconvlen;

#endif
