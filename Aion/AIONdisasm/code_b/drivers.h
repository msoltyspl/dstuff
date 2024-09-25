#ifndef __drivers_h__
#define __drivers_h__

typedef enum _DRVTYPE {
	TEXT, XML
} drvtype;

void DriverInit(drvtype);
void DriverEnema(const char *fname);

#endif
