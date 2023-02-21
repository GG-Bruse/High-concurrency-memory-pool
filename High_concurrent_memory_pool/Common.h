#pragma once
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <cassert>
using std::cout;
using std::endl;

//Win64������_WIN64��_WIN32�����ڣ�Win32������ֻ����_WIN32
#ifdef _WIN64
	typedef unsigned long long PIGE_ID;
#elif _WIN32
	typedef size_t PIGE_ID;
#endif

static const size_t MAX_BYTES = 256 * 1024;//����threadcache���������ֽ���
static const size_t NFREELIST = 208;//Ͱ��
static void*& NextObj(void* obj) { return *(void**)obj; }

class FreeList//�������������ڹ����зֹ���С���ڴ�
{
public:
	void Push(void* obj)
	{
		assert(obj != nullptr);
		NextObj(obj) = _freeList;
		_freeList = obj;
	}
	void* Pop()
	{
		assert(_freeList != nullptr);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		return obj;
	}

	void PushRange(void* start,void* end)//ͷ��һ���ڴ��
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}
	bool IsEmpty() { return _freeList == nullptr; }
	size_t& MaxSize() { return _maxSize; }
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
};



//���ݴ�������
class DataHandleRules
{
	// ������������10%���ҵ�����Ƭ�˷�
	// [1,128]					8byte����	    freelist[0,16)
	// [128+1,1024]				16byte����	    freelist[16,72)
	// [1024+1,8*1024]			128byte����	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte����     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte����   freelist[184,208)

public://���϶������
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


public://ӳ����� : ����ӳ�������һ����������Ͱ
	
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
			return _Index(bytes, 3);//8 ���� 2��3�η�
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

public://һ��thread_cache��central_cache��ȡ���ٸ��ڴ�������ֵ
	static size_t MoveSize(size_t size)
	{
		assert(size > 0);
		// [2, 512] һ�������ƶ����ٸ������(������)����ֵ
		int num = MAX_BYTES / size;
		if (num < 2) num = 2; //�����һ���������޵�
		if (num > 512) num = 512; //С����һ���������޸�
		return num;
	}
};



//�������ҳ�Ŀ�Ƚṹ
struct Span
{
	Span* _prev = nullptr;//˫�������еĽṹ
	Span* _next = nullptr;

	PIGE_ID _pageid = 0;//ҳ��
	size_t _num = 0;//ҳ������

	void* _freeList = nullptr;//��������
	size_t _use_count = 0;//��¼�ѷ����threadcache��ҳ������
};



//��ͷ˫������(Ͱ)
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
	std::mutex mtx;//Ͱ��
};