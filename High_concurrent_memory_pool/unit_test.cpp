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


int main()
{
	TestConcurrentAlloc2();
	return 0;
}