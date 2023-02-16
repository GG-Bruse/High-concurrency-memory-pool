#pragma once
#include "common.h"

class ThreadCache
{
public:
	void* Allocate(size_t);
	void* Deallocate(void* , size_t);
	void* FetchFromCentralCache(size_t index, size_t size);//从中心缓存区获取内存
private:
	FreeList _freelists[NFREELIST];
};