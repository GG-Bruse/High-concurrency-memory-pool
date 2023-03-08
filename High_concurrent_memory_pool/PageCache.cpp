#include "PageCache.h"

PageCache PageCache::_sInst;
PageCache* PageCache::GetInstance() { return &_sInst; }

Span* PageCache::NewSpan(size_t k)//获取一个k页的span
{
	assert(k > 0 && k < NPAGES);

	//检查第k个桶中是否有可用的span
	if (!_spanLists[k].IsEmpty()) return _spanLists[k].PopFront();

	//检查后续的桶中是否有可用的span,若有则进行切分
	for (int i = k + 1; i < NPAGES; ++i) {
		if (!_spanLists[i].IsEmpty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			kSpan->_pageid = nSpan->_pageid;
			nSpan->_pageid += k;
			kSpan->_num = k;
			nSpan->_num -= k;

			_spanLists[nSpan->_num].PushFront(nSpan);

			//存储nSpan的首位页号与nSpan建立映射关系，利于PageCache回收Central_Cache中span时进行合并页
			_idSpanMap[nSpan->_pageid] = nSpan;
			_idSpanMap[nSpan->_pageid + nSpan->_num - 1] = nSpan;
			//建立id和span的映射，方便central cache回收小块内存时，查找对应的span
			for (PAGE_ID i = 0; i < kSpan->_num; ++i) {
				_idSpanMap[kSpan->_pageid + i] = kSpan;
			}

			return kSpan;
		}
	}

	//运行此处则说明PageCache中没有可用的span,需向堆中申请128页的span
	Span* newSpan = new Span;
	void* address = SystemAlloc(NPAGES - 1);
	newSpan->_pageid = (PAGE_ID)address >> PAGE_SHIFT;
	newSpan->_num = NPAGES - 1;
	_spanLists[newSpan->_num].PushFront(newSpan);
	return NewSpan(k);//通过递归将大span分为小span
}

//获取从小内存块到span的映射
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);
	std::unordered_map<PAGE_ID,Span*>::iterator ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end()) {
		return ret->second;
	}
	else {
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{

}