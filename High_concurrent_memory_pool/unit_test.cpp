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

int main()
{
	std::thread t1(Alloc1);
	t1.join();
	std::thread t2(Alloc2);
	t2.join();
	return 0;
}