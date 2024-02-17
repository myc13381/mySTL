#include "Common.h"
#include "PageCache.h"
#include "ConcurrentAlloc.h"

#define TESTALLOCSIZE 10

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_RET_BASE(equality,expect,actual,format)\
do {\
	test_count++;\
	if(equality){test_pass++;}\
	else{\
	fprintf(stderr,"%s:%d: expect:" format " actual:" format "\n",__FILE__,__LINE__,expect,actual);\
	main_ret=1;}\
}while(0)

#define EXPECT_RET_SIZE_T(expect,actual) EXPECT_RET_BASE((expect)==(actual),expect,actual,"%zu")

void TestSize()
{	
	EXPECT_RET_SIZE_T(1, SizeClass::Index(10));
	EXPECT_RET_SIZE_T(1, SizeClass::Index(16));
	EXPECT_RET_SIZE_T(15, SizeClass::Index(128));
	EXPECT_RET_SIZE_T(16, SizeClass::Index(129));
	EXPECT_RET_SIZE_T(17, SizeClass::Index(128+17));
	EXPECT_RET_SIZE_T(72, SizeClass::Index(1025));
	EXPECT_RET_SIZE_T(128, SizeClass::Index(8*1024+1));
	EXPECT_RET_SIZE_T(128, SizeClass::Index(8 * 1024 + 1024));

	EXPECT_RET_SIZE_T(16, SizeClass::Roundup(10));
	EXPECT_RET_SIZE_T(1024 + 128, SizeClass::Roundup(1025));
	EXPECT_RET_SIZE_T(1024 * 8 + 1024, SizeClass::Roundup(1024*8+1));

	std::vector<size_t> v{ 4,8,16,32,64,128,256,512,1024,1024 * 8,1024 * 16,1024 * 64,1024 * 128 };
	for (size_t& s : v)
	{
		cout << s << "\t" << SizeClass::NumMoveSize(s) << "\t" << SizeClass::NumMovePage(s) << endl;
	}
}
std::mutex mtx;
void static Alloc(size_t n, size_t size)//申请和释放测试
{
	size_t begin1 = clock();
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(size));
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();
	size_t end1 = clock();
	//cout << endl << endl;

	//size_t begin2 = clock();
	//for (size_t i = 0; i < n; ++i)
	//{
	//	v.push_back(ConcurrentAlloc(size));
	//}

	//for (size_t i = 0; i < n; ++i)
	//{
	//	ConcurrentFree(v[i]);
	//}
	//v.clear();
	//size_t end2 = clock();

	std::unique_lock<std::mutex> lock(mtx);//防止打印出错
	cout << __func__ << "申请内存次数 " << n << " 每次开辟内存大小 " << size << "byte 耗时 " << end1 - begin1 << endl;
	//cout << __func__ << "申请内存次数 " << n << " 每次开辟内存大小 " << size << "byte 耗时 " << end2 - begin2 << endl;
}

void  static TestThreadCache()
{
	std::thread t1(Alloc, 3,1024*128);
	//std::thread t2(Alloc, 512,100);
	//std::thread t3(Alloc, 50,2048);
	//std::thread t4(Alloc, 1000000,4);

	t1.join();
	//t2.join();
	//t3.join();
	//t4.join();

}

void static TestCentralCache()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 8; ++i)
	{
		v.push_back(ConcurrentAlloc(TESTALLOCSIZE));
	}

	for (size_t i = 0; i < 8; ++i)
	{
		ConcurrentFree(v[i]);
		//cout << v[i]<< endl;
	}
}

void static TestPageCache()
{
	EXPECT_RET_SIZE_T(2*4*1024, PageCache::GetInstence()->NewSpan(2)->_objsize);
	return;
}

void static TestConcurrentAllocFree()
{
	size_t n = 2;
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		void* ptr = ConcurrentAlloc(64*1024+1);
		v.push_back(ptr);
		//printf("obj:%d->%p\n", i, ptr);
		//if (i == 2999999)
		//{
		//	printf("obj:%d->%p\n", i, ptr);
		//}
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
	cout << "hehe" << endl;
}

void static AllocBig()
{
	void* ptr1 = ConcurrentAlloc(65 << PAGE_SHIFT);
	void* ptr2 = ConcurrentAlloc(129 << PAGE_SHIFT);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
}

void static test()
{
	TestSize();
	//Alloc(2,4*1024);
	//TestThreadCache();
	//TestCentralCache();
	//TestPageCache();
	//TestConcurrentAllocFree();
	//AllocBig();

}

//int main()
//{
//	test();
//	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
//	PageCache::GetInstence()->~PageCache();
//	system("pause");
//	return 0;
//}



