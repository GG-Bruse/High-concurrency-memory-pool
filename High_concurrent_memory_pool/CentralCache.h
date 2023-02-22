#pragma once
#include "Common.h"

//饿汉单例模式
class CentralCache
{
public:
	static CentralCache* GetInstance();
	size_t FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size);//从中心缓存获取一定数量的内存块给thread_cache
	Span* GetOneSpan(SpanList& list, size_t size);
private:
	SpanList _spanLists[NFREELIST];
private:
	CentralCache() {}
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;//声明
};