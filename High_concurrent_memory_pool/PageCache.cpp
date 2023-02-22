#include "PageCache.h"

PageCache PageCache::_sInst;
PageCache* PageCache::GetInstance() { return &_sInst; }


Span* PageCache::NewSpan(size_t k)
{

	return nullptr;
}