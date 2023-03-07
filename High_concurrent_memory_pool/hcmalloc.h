#pragma once
#include "Common.h"
#include "ThreadCache.h"

static void* hcmalloc(size_t size)
{
	if (pTLSThreadCache == nullptr) pTLSThreadCache = new ThreadCache;
	//cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
	return pTLSThreadCache->Allocate(size);
}
static void hcfree(void* ptr, size_t size) 
{
	assert(pTLSThreadCache);
	pTLSThreadCache->Deallocate(ptr, size);
}