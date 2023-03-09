#include "CentralCache.h"

CentralCache CentralCache::_sInst;
CentralCache* CentralCache::GetInstance() { return &_sInst; }

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//查看当前CentralCache中的spanlist中是否有还有尚未分配的span
	Span* it = list.Begin();
	while (it != list.End()) {
		if (it->_freeList != nullptr) {
			return it;
		}
		else {
			it = it->_next;
		}
	}
	list._mtx.unlock();//将central_cache桶锁释放，此时若其他线程释放内存回来并不会导致阻塞

	//运行到此处时即没有空闲span，只能向Page_cache索取
	PageCache::GetInstance()->_pageMutex.lock();
	Span* span = PageCache::GetInstance()->NewSpan(DataHandleRules::NumMovePage(size));
	span->_IsUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMutex.unlock();

	//计算span的大块内存的起始地址和大块内存的大小(字节数)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_num << PAGE_SHIFT;
	char* end = start + bytes;

	//将大块内存切成自由链表并链接起来
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}

	list._mtx.lock();
	list.PushFront(span);//将span挂入对应的桶中
	return span;
}



size_t CentralCache::FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = DataHandleRules::Index(size);
	_spanLists[index]._mtx.lock();
	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	//从span中获取batchNum块内存块，若不够则有多少获取多少
	start = span->_freeList;
	end = span->_freeList;
	size_t count = 0, actualNum = 1;
	while (NextObj(end) != nullptr && count < batchNum - 1) {
		end = NextObj(end);
		++count;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->_use_count += actualNum;

	_spanLists[index]._mtx.unlock();
	return actualNum;
}



void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t bucketIndex = DataHandleRules::Index(size);
	_spanLists[bucketIndex]._mtx.lock();
	while (start) 
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		NextObj(start) = span->_freeList;
		span->_freeList = start;
		span->_use_count--;

		if (span->_use_count == 0) //说明该span切分的内存块都已经归还了，该span可以归还给PageCache
		{
			_spanLists[bucketIndex].Erase(span);
			span->_freeList = nullptr;//看作整体，可使用页号转换为地址来找到内存
			span->_next = nullptr;
			span->_prev = nullptr;

			_spanLists[bucketIndex]._mtx.unlock();
			PageCache::GetInstance()->_pageMutex.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMutex.unlock();
			_spanLists[bucketIndex]._mtx.lock();
		}
		start = next;
	}
	_spanLists[bucketIndex]._mtx.unlock();
}