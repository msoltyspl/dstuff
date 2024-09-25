#ifndef __textconv_h__
#define __textconv_h__

#include <stdint.h>

int textconv_init(int siz);
void textconv_destroy(void);
//int utf8rem_bs(uint8_t* to, const uint8_t* from,int tocnt,int fromcnt);
int utf8add_bs(uint8_t* to, const uint8_t* from,int tolim,int fromcnt, int extras);
int textconv_conv(const char *ocode, uint8_t **obuf, int *osiz, const char *icode, uint8_t* ibuf, const int isiz, int dotrans, const int loud);
void textconv_summary(void);

#endif
