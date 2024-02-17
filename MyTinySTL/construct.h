#ifndef MYTINYSTL_CONSTRUCT_H_
#define MYTINYSTL_CONSTRUCT_H_

// 这个头文件包含两个函数 construct，destroy
// construct : 负责对象的构造
// destroy   : 负责对象的析构

#include <new>

#include "type_traits.h"
#include "iterator.h"

// 使用宏控制警告输出 Microsotf Visual C++ 编译环境下
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)  // unused parameter
#endif // _MSC_VER

namespace mystl
{

// construct 构造对象
// 使用定位new，在指定位置创建新的对象，该位置需要已经预分配了内存空间
template <class Ty>
void construct(Ty* ptr)
{
  ::new ((void*)ptr) Ty();
}

template <class Ty1, class Ty2>
void construct(Ty1* ptr, const Ty2& value)
{
  ::new ((void*)ptr) Ty1(value);
}


// // 判断Ty2类型能否转化为Ty1
// template <class Ty1, class Ty2, 
//   typename = typename std::enable_if<std::is_convertible<Ty2,Ty1>::value,void>::type> 
// void construct(Ty1* ptr, const Ty2& value)
// {
//   ::new ((void*)ptr) Ty1(value);
// }

template <class Ty, class... Args>
void construct(Ty* ptr, Args&&... args)
{
  ::new ((void*)ptr) Ty(mystl::forward<Args>(args)...);// 展开模板参数包,作为构造函数的参数
}

// destroy 将对象析构

template <class Ty>
void destroy_one(Ty*, std::true_type) {}

template <class Ty>
void destroy_one(Ty* pointer, std::false_type)
{
  if (pointer != nullptr)
  {
    pointer->~Ty();
  }
}

template <class ForwardIter>
void destroy_cat(ForwardIter , ForwardIter , std::true_type) {}

template <class ForwardIter>
void destroy_cat(ForwardIter first, ForwardIter last, std::false_type)
{
  for (; first != last; ++first)
    destroy(&*first);
}


// std::is_trivially_destructible<Ty>{}用于判断类型Ty是否是平凡可析构类型，如果是则返回std::true_type,如果不是返回std::false_type
// 如果Ty是平凡析构类型，意味着其析构函数将不会执行任何操作，因此可以不显式调用析构函数
template <class Ty>
void destroy(Ty* pointer)
{
  destroy_one(pointer, std::is_trivially_destructible<Ty>{});
}

template <class ForwardIter>
void destroy(ForwardIter first, ForwardIter last)
{
  destroy_cat(first, last, std::is_trivially_destructible<
              typename iterator_traits<ForwardIter>::value_type>{});
}

} // namespace mystl

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // !MYTINYSTL_CONSTRUCT_H_

