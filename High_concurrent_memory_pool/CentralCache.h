#pragma once
#include "Common.h"

//��������ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance();
	size_t FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size);//�����Ļ����ȡһ���������ڴ���thread_cache

private:
	SpanList _freeLists[NFREELIST];
private:
	CentralCache() {}
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;//����
};