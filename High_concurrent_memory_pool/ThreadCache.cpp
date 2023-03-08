#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size) 
{
	assert(size <= MAX_BYTES);
	size_t alignSize = DataHandleRules::AlignUp(size);
	size_t bucketIndex = DataHandleRules::Index(size);
	if (!_freeLists[bucketIndex].IsEmpty()){
		return _freeLists[bucketIndex].Pop();
	}
	else {
		return FetchFromCentralCache(bucketIndex, alignSize);
	}
}
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr != nullptr);
	assert(size <= MAX_BYTES);
	size_t bucketIndex = DataHandleRules::Index(size);
	_freeLists[bucketIndex].Push(ptr);

	//当链表长度大于一次批量申请的内存时就归还一段自由链表上的内存给CentralCache
	if (_freeLists[bucketIndex].Size() == _freeLists[bucketIndex].MaxSize())
		ListTooLong(_freeLists[bucketIndex], size);
}



void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	//慢开始反馈调节算法
	//并不会一开始一批量向central_cache索要太多，可能使用不完
	size_t batchNum = min(_freeLists[index].MaxSize(), DataHandleRules::MoveSize(size));
	if (batchNum == _freeLists[index].MaxSize()) {
		_freeLists[index].MaxSize() += 2;//若不断需要size大小的内存，那么batchNum就会不断增长直至上限
	}

	void* start = nullptr, * end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchMemoryBlock(start, end, batchNum, size);
	assert(actualNum > 0);//至少分配一个

	if (actualNum == 1) {
		assert(start == end);
		return start;
	}
	else{
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);//将后面的内存头插thread_cache自由链表中
		return start;//将第一个内存块返回给外面使用
	}
}



void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr, * end = nullptr;
	list.PopRange(start, end, list.MaxSize());
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}