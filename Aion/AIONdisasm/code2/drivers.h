#ifndef __drivers_h__
#define __drivers_h__

typedef enum _DRVTYPE {
//	XML, TEXT
	TEXT, XML
} drvtype;

//extern drvtype g_curr_driver;

int DriverInit(drvtype);
int DriverOut(const char*);

#endif
