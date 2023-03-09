#include "hcmalloc.h"

void Alloc1()
{
	for (size_t i = 0; i < 5; ++i) {
		void* ptr = hcmalloc(6);
	}
}
void Alloc2()
{
	for (size_t i = 0; i < 5; ++i) {
		void* ptr = hcmalloc(7);
	}
}

void TestAlloc()
{
	std::thread t1(Alloc1);
	t1.join();
	std::thread t2(Alloc2);
	t2.join();
}

void TestConcurrentAlloc1()
{
	void* p1 = hcmalloc(6);
	void* p2 = hcmalloc(8);
	void* p3 = hcmalloc(1);
	void* p4 = hcmalloc(7);
	void* p5 = hcmalloc(8);
	cout << p1 << endl;
	cout << p2 << endl;
	cout << p3 << endl;
	cout << p4 << endl;
	cout << p5 << endl;
	hcfree(p1);
	hcfree(p2);
	hcfree(p3);
	hcfree(p4);
	hcfree(p5);
}
void TestConcurrentAlloc2()
{
	for (size_t i = 0; i < 1024; ++i)
	{
		void* p1 = hcmalloc(6);
		cout << p1 << endl;
	}
	void* p2 = hcmalloc(8);
	cout << p2 << endl;
}


void MultiThreadAlloc1()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 7; ++i)
	{
		void* ptr = hcmalloc(6);
		v.push_back(ptr);
	}

	for (auto e : v) {
		hcfree(e);
	}
}
void MultiThreadAlloc2()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 7; ++i)
	{
		void* ptr = hcmalloc(16);
		v.push_back(ptr);
	}

	for (auto e : v) {
		hcfree(e);
	}
}

void TestMultiThread()
{
	std::thread t1(MultiThreadAlloc1);
	std::thread t2(MultiThreadAlloc2);

	t1.join();
	t2.join();
}


void BigAlloc()
{
	void* p1 = hcmalloc(257 * 1024);
	hcfree(p1);

	void* p2 = hcmalloc(129 * 8 * 1024);
	hcfree(p2);
}
int main()
{
	TestConcurrentAlloc1();
	TestConcurrentAlloc2();
	TestMultiThread();
	BigAlloc();

	return 0;
}