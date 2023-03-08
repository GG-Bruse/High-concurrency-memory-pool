#pragma once
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <cassert>
using std::cout;
using std::endl;

#ifdef _WIN32
	#include <windows.h>
#else
	// ...
#endif

//Win64环境下_WIN64和_WIN32都存在，Win32环境下只存在_WIN32
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else//Linux
	//...
#endif

static const size_t MAX_BYTES = 256 * 1024;//能在threadcache申请的最大字节数
static const size_t NFREELIST = 208;//thread_cache && central_cache 桶数
static const size_t NPAGES = 129;//page_cache的桶数+1 || page_cache的最大页数+1 (下标为0位置空出)
static const size_t PAGE_SHIFT = 13;
static void*& NextObj(void* obj) { return *(void**)obj; }

inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux下brk mmap等
#endif
	if (ptr == nullptr) throw std::bad_alloc();
	return ptr;
}



class FreeList//自由链表：用于管理切分过的小块内存
{
public:
	void Push(void* obj)
	{
		assert(obj != nullptr);
		NextObj(obj) = _freeList;
		_freeList = obj;
		++_size;
	}
	void* Pop()
	{
		assert(_freeList != nullptr);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		--_size;
		return obj;
	}

	void PushRange(void* start, void* end, size_t n)//头插一段内存块
	{
		NextObj(end) = _freeList;
		_freeList = start;
		_size += n;
	}
	void PopRange(void*& start, void*& end, size_t n) 
	{
		assert(n <= _size);
		start = _freeList;
		end = start;
		for (size_t i = 0; i < n; ++i) {
			end = NextObj(end);
		}
		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n;
	}

	bool IsEmpty() { return _freeList == nullptr; }
	size_t& MaxSize() { return _maxSize; }
	size_t Size() { return _size; }
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
	size_t _size = 0;
};



//数据处理规则
class DataHandleRules
{
	// 整体控制在最多10%左右的内碎片浪费
	// [1,128]					8byte对齐	    freelist[0,16)
	// [128+1,1024]				16byte对齐	    freelist[16,72)
	// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)

public://向上对齐规则
	/*static inline size_t _AlignUp(size_t size, size_t alignNum)
	{
		size_t alignRet = 0;
		if (size % alignNum != 0) {
			alignRet = ((size / alignNum) + 1) * alignNum;
		}
		else {
			alignRet = size;
		}
		return alignRet;
	}*/
	static inline size_t _AlignUp(size_t bytes, size_t alignNum) { return ((bytes + alignNum - 1) & ~(alignNum - 1)); }

	static inline size_t AlignUp(size_t size)
	{
		if (size < 128) return _AlignUp(size, 8);
		else if (size < 1024) return _AlignUp(size, 16);
		else if (size < 8 * 1024) return _AlignUp(size, 128);
		else if (size < 64 * 1024) return _AlignUp(size, 1024);
		else if (size < 256 * 1024) return _AlignUp(size, 8 * 1024);
		else exit(-1);
	}


public://映射规则 : 计算映射的是哪一个自由链表桶
	
	/*size_t _Index(size_t bytes, size_t alignNum)
	{
		if (bytes % alignNum == 0) {
			return bytes / alignNum - 1;
		}
		else {
			return bytes / alignNum;
		}
	}*/
	//...

	static inline size_t _Index(size_t bytes, size_t align_shift) { return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1; }

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);//8 等于 2的3次方
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
		}

		return -1;
	}

public://一次thread_cache从central_cache获取多少个内存块的上限值
	static size_t MoveSize(size_t size)
	{
		assert(size > 0);
		// [2, 512] 一次批量移动多少个对象的(慢启动)上限值
		int num = MAX_BYTES / size;
		if (num < 2) num = 2; //大对象一次批量上限低
		if (num > 512) num = 512; //小对象一次批量上限高
		return num;
	}

public://一次central_cache向page_cache获取多少个页
	static size_t NumMovePage(size_t size)
	{
		size_t num = MoveSize(size);
		size_t npage = num * size;

		npage >>= PAGE_SHIFT;
		if (npage == 0) npage = 1;

		return npage;
	}
};



//管理多个页的跨度结构
struct Span
{
	Span* _prev = nullptr;//双向链表中的结构
	Span* _next = nullptr;

	PAGE_ID _pageid = 0;//页号
	size_t _num = 0;//页的数量

	void* _freeList = nullptr;//自由链表
	size_t _use_count = 0;//记录已分配给threadcache的页的数量
};



//带头双向链表(桶)
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		assert(_head != nullptr);
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin() { return _head->_next; }
	Span* End() { return _head; }
	bool IsEmpty() { return _head == _head->_next; }
	void PushFront(Span* span) { Insert(Begin(), span); }
	Span* PopFront() { 
		Span* front = _head->_next;
		Erase(front);
		return front;
	}

	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);
		Span* prev = pos->_prev;
		newSpan->_next = pos;
		prev->_next = newSpan;
		newSpan->_prev = prev;
		pos->_prev = newSpan;

	}
	void Erase(Span* pos)
	{
		assert(pos != nullptr);
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

private:
	Span* _head = nullptr;
public:
	std::mutex _mtx;//桶锁
};