#ifndef __cfgblob_h__
#define __cfgblob_h__

#include <stdint.h>
#include "bfile.h"

int ProcessCFGBlob(BFdesc *fin, uint32_t chr);

extern uint8_t *g_cfgconvptr;
extern int g_cfgconvlen;

#endif
