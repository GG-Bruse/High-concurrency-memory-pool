#pragma once
#include "Common.h"

//饿汉单例模式
class PageCache
{
public:
	static PageCache* GetInstance();
	Span* NewSpan(size_t k);
	Span* MapObjectToSpan(void* obj);
	void ReleaseSpanToPageCache(Span* span);
private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	static PageCache _sInst;
	SpanList _spanLists[NPAGES];
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;
public:
	std::mutex _pageMutex;//整个page_cache的锁
};