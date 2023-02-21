#include "CentralCache.h"

CentralCache CentralCache::_sInst;
CentralCache* CentralCache::GetInstance() { return &_sInst; }

size_t CentralCache::FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = DataHandleRules::Index(size);
	//...
	return 10;
}
