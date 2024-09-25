#ifndef __drivers_h__
#define __drivers_h__

typedef enum _DRVTYPE {
	TEXT, XML
} drvtype;

int DriverInit(drvtype);
int DriverOut(const char*);

#endif
