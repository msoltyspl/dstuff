#ifndef __xmlblob_h__
#define __xmlblob_h__

#include "pooler.h"
#include "hashptr.h"

typedef struct {
	int key, val;
} XML_PAIR;

typedef struct _XML_NODE {
	int name, val, nattr, nsubs;
	XML_PAIR *attr;
	struct _XML_NODE **subs;
} XML_NODE;

typedef struct {
	char *str;
	int siz;
} STR_NODE;

extern XML_NODE *g_xb_root;
extern STR_NODE *g_xb_strptr;
extern int g_xb_indent;

int ProcessBlob(const char*,int);

#endif
