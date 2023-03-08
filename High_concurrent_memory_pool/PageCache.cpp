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

			kSpan->_pageId = nSpan->_pageId;
			nSpan->_pageId += k;
			kSpan->_num = k;
			nSpan->_num -= k;

			_spanLists[nSpan->_num].PushFront(nSpan);

			//存储nSpan的首尾页号与nSpan建立映射关系，利于PageCache回收Central_Cache中span时进行合并页
			_idSpanMap[nSpan->_pageId] = nSpan;
			_idSpanMap[nSpan->_pageId + nSpan->_num - 1] = nSpan;
			//建立id和span的映射，方便central cache回收小块内存时，查找对应的span
			for (PAGE_ID i = 0; i < kSpan->_num; ++i) {
				_idSpanMap[kSpan->_pageId + i] = kSpan;
			}

			return kSpan;
		}
	}

	//运行此处则说明PageCache中没有可用的span,需向堆中申请128页的span
	Span* newSpan = new Span;
	void* address = SystemAlloc(NPAGES - 1);
	newSpan->_pageId = (PAGE_ID)address >> PAGE_SHIFT;
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
	while (1)//向前合并
	{
		PAGE_ID prevId = span->_pageId - 1;
		std::unordered_map<PAGE_ID, Span*>::iterator ret = _idSpanMap.find(prevId);
		if (ret == _idSpanMap.end()) break;

		//前面相邻页的span正在使用中
		Span* prevSpan = ret->second;
		if (prevSpan->_IsUse == true) break;

		if (prevSpan->_num + span->_num > NPAGES - 1) break;//若合并后大于128页则无法管理

		span->_pageId = prevSpan->_pageId;
		span->_num += prevSpan->_num;
		_spanLists[prevSpan->_num].Erase(prevSpan);
		delete prevSpan;
	}
	while (1)//向后合并
	{
		PAGE_ID nextId = span->_pageId + span->_num;
		std::unordered_map<PAGE_ID, Span*>::iterator ret = _idSpanMap.find(nextId);
		if (ret == _idSpanMap.end()) break;

		//后面相邻页的span正在使用中
		Span* nextSpan = ret->second;
		if (nextSpan->_IsUse == true) break;

		if (nextSpan->_num + span->_num > NPAGES - 1) break;//若合并后大于128页则无法管理

		span->_num += nextSpan->_num;
		_spanLists[nextSpan->_num].Erase(nextSpan);
		delete nextSpan;
	}

	_spanLists[span->_num].PushFront(span);
	_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_num - 1] = span;
	span->_IsUse = false;
}