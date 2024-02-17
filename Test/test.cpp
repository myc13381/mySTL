#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC 
#include <stdlib.h>
#include <crtdbg.h>
#endif // check memory leaks

#include "algorithm_performance_test.h"
#include "algorithm_test.h"
#include "vector_test.h"
#include "list_test.h"
#include "deque_test.h"
#include "queue_test.h"
#include "stack_test.h"
#include "map_test.h"
#include "set_test.h"
#include "unordered_map_test.h"
#include "unordered_set_test.h"
#include "string_test.h"
#include "vector.h"

int main()
{
  system("clear");
  using namespace mystl::test;

  std::cout.sync_with_stdio(false); // 这行代码将std::cout与C风格的I/O设置为不同步。这意味着当你使用std::cout时，它的缓冲行为不会受到C风格I/O的影响。这有助于避免输出混乱的问题

  mystl::vector<int> v {1,4,6,7,3,2};
  mystl::sort(v.begin(),v.end());
  for(int i=0;i<5;++i)
    cout<<v[i]<<endl;
  v.emplace_back(11);
  RUN_ALL_TESTS(); // 包含了 mystl 的 81 个算法测试
  // // 容器测试
  algorithm_performance_test::algorithm_performance_test();
  vector_test::vector_test();
   list_test::list_test();
  deque_test::deque_test();
  queue_test::queue_test();
  queue_test::priority_test();
  stack_test::stack_test();
  map_test::map_test();
  map_test::multimap_test();
  set_test::set_test();
  set_test::multiset_test();
  unordered_map_test::unordered_map_test();
  unordered_map_test::unordered_multimap_test();
  unordered_set_test::unordered_set_test();
  unordered_set_test::unordered_multiset_test();
  string_test::string_test();

#if defined(_MSC_VER) && defined(_DEBUG)
  _CrtDumpMemoryLeaks();
#endif // check memory leaks

}
