#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "pooler.h"

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

//#define CHUNK_UNIT (2097152u)
#define CHUNK_UNIT (1048576u)
#define CHUNK_SIZ (1u*CHUNK_UNIT)
#define POOL_CNT 128u
#define HUGE_THRESHOLD (131072u)
//#define ALIGN 4

static const char g_reall_err[]="Pooler failure: realloc call.\n";

static const char g_mall_err[]="Pooler failure (norm): malloc call.\n";
static const char g_hmall_err[]="Pooler failure (huge): malloc call.\n";

//static void *HPoolerAssign(POOLER* const pool, const uintptr_t siz)

void *HPoolerAssign(POOLER* const pool, const uintptr_t siz)
{
	void* ret = 0;

	if(pool->hcur + 1 == pool->hcnt_resrv) {
		if(!(pool->hptr = (void**)realloc(pool->hptr, pool->hcnt_resrv += POOL_CNT))) {
			fputs(g_reall_err,stderr);
			EXCEPT;
		}
	}
	if(pool->hcur + 1 == pool->hcnt_alloc) {
		if(!(pool->hptr[++pool->hcur] = malloc(siz))) {
			fputs(g_hmall_err,stderr);
			EXCEPT;
		}
		pool->hcnt_alloc++;
	}

	pool->hmalloc_total += siz;
	pool->thmalloc_total += siz;
	ret = pool->hptr[pool->hcur];

EXCEPT_LABEL;
	return ret;
}

void *PoolerAssign(POOLER* const pool, const uintptr_t siz)
{
	uintptr_t tmp_inoff;
	void* ret = 0;

/*
	if(siz > CHUNK_SIZ) {
		fprintf(stderr,"Pooler refuses to allocate more than %u MiB at once. Check your ddf files for errors.\nIf desperate, ask on forums.",CHUNK_SIZ/CHUNK_UNIT);
		EXCEPT;
	}
*/

	if(siz > HUGE_THRESHOLD) {
		return HPoolerAssign(pool, siz);
	}

	//align offset to sizeof(uintptr_t) bytes boundary
	pool->inoff = (pool->inoff+(sizeof(uintptr_t)-1u)) & (intptr_t)(-sizeof(uintptr_t));

/*
	if we're beyond chunk size, we must alloc more memory
	->cur holds current bucket. If it's not enough, we must upsize bucket table
	->cnt_resrv holds current size of bucket table
	->cnt_alloc holds current amount of actually allocated buckets
*/

	if(pool->inoff + siz > CHUNK_SIZ) {
		if(pool->cur + 1 == pool->cnt_resrv) {
			if(!(pool->ptr = (void**)realloc(pool->ptr, pool->cnt_resrv += POOL_CNT))) {
				fputs(g_reall_err,stderr);
				EXCEPT;
			}
		}
		if(pool->cur + 1 == pool->cnt_alloc) {
			if(!(pool->ptr[++pool->cur] = malloc(CHUNK_SIZ))) {
				fputs(g_mall_err,stderr);
				EXCEPT;
			}
			pool->cnt_alloc++;
		}
		pool->inoff = 0;
	}
	tmp_inoff = pool->inoff;
	pool->inoff += siz;
	ret = (uint8_t*)pool->ptr[pool->cur] + tmp_inoff;

EXCEPT_LABEL;
	return ret;
}

POOLER *PoolerInit(const char *name)
{
	POOLER *pool, *ret = NULL;

	if(!(pool=(POOLER *)malloc(sizeof(POOLER))))
		EXCEPT;

	pool->cur = 0;
	pool->inoff = 0;
	pool->cnt_resrv = POOL_CNT;
	pool->cnt_alloc = 1;

	//note that regular pooler may not use negative values
	pool->hcur = -1;
	pool->hcnt_resrv = 0;
	pool->hcnt_alloc = 0;
	pool->hmalloc_total = 0;
	pool->thmalloc_total = 0;

	if(!(pool->ptr = (void**)malloc(pool->cnt_resrv*sizeof(void*))))
		EXCEPT;

	pool->hptr = NULL;

	if(!(pool->ptr[0] = malloc(CHUNK_SIZ)))
		EXCEPT;

	strncpy(pool->name,name,31);
	pool->name[31]=0;

	ret = pool;
EXCEPT_LABEL;
	if(!ret) {
		fputs(g_mall_err,stderr);
		if(pool) { 
			if(pool->ptr)
				free(pool->ptr);
			free(pool);
		}
	}
	return ret;
}

//FIX
void PoolerFree(POOLER * const pool)
{
	pool->cur = 0;
	pool->inoff = 0;
	pool->hmalloc_total = 0;
	for(int i=0;i<pool->hcnt_alloc;i++) {
		free(pool->hptr[i]);
	}
}

void PoolerDestroy(POOLER * const pool)
{
	int i;
	if(!pool)
		return;
//	#ifndef NDEBUG
	printf("Total allocated by pooler currenty (norm) \"%s\": %d\n", pool->name,
		pool->cnt_resrv*sizeof(void*) + pool->cnt_alloc*CHUNK_SIZ
	);
	printf("Total allocated by pooler currenty (huge) \"%s\": %d\n", pool->name,
		pool->hcnt_resrv*sizeof(void*) + pool->hmalloc_total
	);
	printf("Total rotated by pooler (huge) \"%s\": %d\n", pool->name,
		pool->hcnt_resrv*sizeof(void*) + pool->thmalloc_total
	);
//	#endif
	for(i=0;i<pool->cnt_alloc;i++) {
		free(pool->ptr[i]);
	}
	for(i=0;i<pool->hcnt_alloc;i++) {
		free(pool->hptr[i]);
	}
	free(pool->ptr);
	free(pool->hptr);
	free(pool);
}



	
//	#if RELEASE == 0
//		printf("Total allocated by pooler: %d\n", pool_siz*sizeof(void*) + (pool_cur+1)*CHUNK_SIZ);
//	#endif
//}
