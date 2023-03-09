#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

static void* hcmalloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		size_t alignSize = DataHandleRules::AlignUp(size);
		size_t kPage = alignSize >> PAGE_SHIFT;

		PageCache::GetInstance()->_pageMutex.lock();
		Span* span = PageCache::GetInstance()->NewSpan(kPage);
		span->_objSize = size;
		PageCache::GetInstance()->_pageMutex.unlock();

		void* address = (void*)(span->_pageId << PAGE_SHIFT);
		return address;
	}
	else
	{
		static ObjectPool<ThreadCache> threadCachePool;
		if (pTLSThreadCache == nullptr) pTLSThreadCache = threadCachePool.New();
		cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
		return pTLSThreadCache->Allocate(size);
	}
}
static void hcfree(void* ptr) 
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objSize;
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->_pageMutex.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMutex.unlock();
	}
	else
	{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}