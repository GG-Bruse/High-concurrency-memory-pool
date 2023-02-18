#include "threadcache.h"

void* ThreadCache::Allocate(size_t size) 
{
	assert(size <= MAX_BYTES);
	size_t alignSize = AlignMappingRules::AlignUp(size);
	size_t bucketIndex = AlignMappingRules::Index(size);
	if (!_freelists[bucketIndex].IsEmpty()){
		return _freelists[bucketIndex].Pop();
	}
	else {
		return FetchFromCentralCache(bucketIndex, alignSize);
	}
}
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr != nullptr);
	assert(size <= MAX_BYTES);
	size_t bucketIndex = AlignMappingRules::Index(size);
	_freelists[bucketIndex].Push(ptr);
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	//...
	return nullptr;
}

