#pragma once
#include "Common.h"

class ThreadCache
{
	friend static void ThreadCacheClean();
public:
	void* Allocate(size_t);
	void Deallocate(void* , size_t);
	void* FetchFromCentralCache(size_t index, size_t size);//从中心缓存区获取内存
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList _freeLists[NFREELIST];
};

// TLS thread local storage
//static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;
static thread_local ThreadCache* pTLSThreadCache = nullptr;