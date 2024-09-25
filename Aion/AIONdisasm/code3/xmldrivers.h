#ifndef __xmldrivers_h__
#define __xmldrivers_h__
#include "bfile.h"

typedef enum _DRVTYPE {
//	XML, TEXT
	TEXT, XML, SQL
} drvtype;

//extern drvtype g_curr_driver;

int XMLDriverInit(drvtype);
void XMLDriverDestroy(void);
int XMLDriverOut(BFdesc *fout);

#endif
