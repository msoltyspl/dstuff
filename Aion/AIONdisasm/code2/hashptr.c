#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "hashptr.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

HTAB* HashInit(int siz)
{
	HTAB *ptr = NULL, *ret = NULL;
	int siz2 = 1;

	if(!(ptr = (HTAB *)malloc(sizeof(HTAB))))
		EXCEPT;

	while(siz>siz2)
		siz2+=siz2;

	if(!(ptr->ptr = (HTAB_NODE*)malloc(siz2*sizeof(HTAB_NODE))))
		EXCEPT;

	ptr->hash_cols = 0;
	ptr->n = siz2;
	memset(ptr->ptr,-1,siz2*sizeof(HTAB_NODE));

	ret = ptr;
EXCEPT_LABEL;
	if(!ret) {
		if(ptr)
			free(ptr);
	}

	return ret;
}

HTAB *HashReinit(HTAB * const htab, int siz)
{
	HashDestroy(htab);
	return HashInit(siz);
}


void HashDump(HTAB * const htab)
{
	if(!htab)
		return;
	printf("Max collisions: %d\n", htab->hash_cols);
	for(int i=0;i<htab->n;i++) {
		if(htab->ptr[i].key != UINTPTR_MAX)
			printf("%u(%x) -> %d\n",htab->ptr[i].key,htab->ptr[i].key,htab->ptr[i].val);
	}
}

void HashDestroy(HTAB * const htab)
{
	if(!htab)
		return;
	if(htab->ptr)
		free(htab->ptr);
	free(htab);
}

#if __SIZEOF_POINTER__ == 8
static uintptr_t lhash(uintptr_t  key)
{
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = key * 265ULL;
	key = key ^ (key >> 14);
	key = key * 21ULL;
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}
#else
static uintptr_t lhash(uintptr_t key)
{
	key = (~key) + (key << 15);
	key = key ^ (key >> 12);
	key = key + (key << 2);
	key = key ^ (key >> 4);
	key = key * 2057u;
	key = key ^ (key >> 16);
	return key;
}
#endif

int HashSet(HTAB * const htab, const uintptr_t dat, const uintptr_t val)
{
	int h, d = 1, cnt = 0;

	h = lhash(dat) & (htab->n - 1);

	while(1) {
        	if (htab->ptr[h].key == dat || htab->ptr[h].key == UINTPTR_MAX) { //found or empty
        		htab->ptr[h].key = dat;
        		htab->ptr[h].val = val;
        		return 0;
        	} else { //quadratic probing x(x+1)/2
//        		if(d > htab->n)
  //      			return -1;
			if(++cnt >= 64)
				return -1;
			if(cnt > htab->hash_cols)
				htab->hash_cols = cnt;
        		h = (h + d++) & (htab->n - 1);
        	}
	}
}

uintptr_t HashTest(const HTAB * const htab, const uintptr_t dat)
{
	int h, d = 1, cnt = 0;

	h = lhash(dat) & (htab->n - 1);

	while(1) {
        	if (htab->ptr[h].key == dat) { //found
        		return htab->ptr[h].val;
        	} else if(htab->ptr[h].key == UINTPTR_MAX) { //not found
        		return UINTPTR_MAX;
        	} else { //quadratic probing x(x+1)/2
//        		if(d > htab->n)
  //      			return -1;
			if(++cnt >= 128)
				return UINTPTR_MAX;
        		h = (h + d++) & (htab->n - 1);
        	}
	}
}
