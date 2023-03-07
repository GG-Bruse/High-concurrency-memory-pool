#pragma once
#include "Common.h"

//饿汉单例模式
class PageCache
{
public:
	static PageCache* GetInstance();
	Span* NewSpan(size_t k);

private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	static PageCache _sInst;
	SpanList _spanLists[NPAGES];
public:
	std::mutex _pageMutex;//整个page_cache的锁
};