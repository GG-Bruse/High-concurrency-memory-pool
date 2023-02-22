#include "CentralCache.h"

CentralCache CentralCache::_sInst;
CentralCache* CentralCache::GetInstance() { return &_sInst; }

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//...
	return nullptr;
}

size_t CentralCache::FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = DataHandleRules::Index(size);
	_spanLists[index]._mtx.lock();
	Span* span = GetOneSpan(_spanLists[index], batchNum);
	assert(span);
	assert(span->_freeList);

	start = span->_freeList;
	end = start;
	size_t count = 0, actualNum = 1;
	while (count <= batchNum - 1 && NextObj(end) != nullptr) {
		end = NextObj(end);
		++count;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanLists[index]._mtx.unlock();
	return actualNum;
}
