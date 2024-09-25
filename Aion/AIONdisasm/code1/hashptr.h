#ifndef __hashptr_h__
#define __hashptr_h__

#include <stdint.h>

typedef struct {
	uintptr_t key, val;
} HTAB_NODE;


typedef struct {
	HTAB_NODE *ptr;
	int n, hash_cols;
} HTAB;

HTAB* HashInit(int siz);
HTAB *HashReinit(HTAB * const htab, int siz);
void HashDump(HTAB * const htab);
void HashDestroy(HTAB * const htab);
int HashSet(HTAB * const htab, const uintptr_t dat, const uintptr_t val);
uintptr_t HashTest(const HTAB * const htab, const uintptr_t dat);

#endif
