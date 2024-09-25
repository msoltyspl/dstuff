#ifndef __pooler_h__
#define __pooler_h__

#include <stdint.h>

typedef struct {
	void **ptr;
	uintptr_t inoff;	//huge doesn't use it
	uint32_t cur;
	uint32_t cnt_resrv;
	uint32_t cnt_alloc;
//	uint32_t hcur;
//	uint32_t hcnt_resrv;
//	uint32_t hcnt_alloc;
	char name[32];
} POOLER;


POOLER *PoolerInit(const char* name);
void *PoolerAssign(POOLER*const, const uintptr_t);
void PoolerFree(POOLER *const);
void PoolerDestroy(POOLER * const);

#endif
