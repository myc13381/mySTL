#ifndef MYTINYSTL_ALLOCATOR_H_
#define MYTINYSTL_ALLOCATOR_H_

// 这个头文件包含一个模板类 allocator，用于管理内存的分配、释放，对象的构造、析构

#include "construct.h"
#include "util.h"

#include "Common.h"
#include "ConcurrentAlloc.h"

//#define USE_ALLOCTOR_MEM // 自己实现的内存池有bug 大量数据会出错

namespace mystl
{

// 模板类：allocator
// 模板函数代表数据类型
template <class T>
class allocator
{
public:
  typedef T            value_type;
  typedef T*           pointer;
  typedef const T*     const_pointer;
  typedef T&           reference;
  typedef const T&     const_reference;
  typedef size_t       size_type;
  typedef ptrdiff_t    difference_type;

public:
  // function
  // allocate 使用 ::operator new (size) 来申请内存空间
  static T*   allocate();
  static T*   allocate(size_type n);

  // deallocate 使用 ::operator delete(ptr) 来释放空间
  static void deallocate(T* ptr);
  static void deallocate(T* ptr, size_type n);

  // 调用 construct.h中实现的construct函数
  static void construct(T* ptr);
  static void construct(T* ptr, const T& value);
  static void construct(T* ptr, T&& value);

  template <class... Args>
  static void construct(T* ptr, Args&&... args);

  // 调用construct.h中的destroy函数
  static void destroy(T* ptr);
  static void destroy(T* first, T* last);
};

// 使用全局范围的new关键字
template <class T>
T* allocator<T>::allocate()
{
#ifdef USE_ALLOCTOR_MEM
  return static_cast<T*>(ConcurrentAlloc(sizeof(T)));
#else
  return static_cast<T*>(::operator new(sizeof(T)));
#endif
}

template <class T>
T* allocator<T>::allocate(size_type n)
{
  if (n == 0)
    return nullptr;
#ifdef USE_ALLOCTOR_MEM
  return static_cast<T*>(ConcurrentAlloc(n * sizeof(T)));
#else
  return static_cast<T*>(::operator new(n * sizeof(T)));
#endif
}

template <class T>
void allocator<T>::deallocate(T* ptr)
{
  if (ptr == nullptr)
    return;
#ifdef USE_ALLOCTOR_MEM
  ConcurrentFree(static_cast<void*>(ptr));
#else
  ::operator delete(ptr);
#endif
}

template <class T>
void allocator<T>::deallocate(T* ptr, size_type /*size*/)
{
  if (ptr == nullptr)
    return;
#ifdef USE_ALLOCTOR_MEM
  ConcurrentFree(static_cast<void*>(ptr));
#else
  ::operator delete(ptr);
#endif
}

template <class T>
void allocator<T>::construct(T* ptr)
{
  mystl::construct(ptr);
}

template <class T>
void allocator<T>::construct(T* ptr, const T& value)
{
  mystl::construct(ptr, value);
}

template <class T>
 void allocator<T>::construct(T* ptr, T&& value)
{
  mystl::construct(ptr, mystl::move(value));
}

template <class T>
template <class ...Args>
 void allocator<T>::construct(T* ptr, Args&& ...args)
{
  mystl::construct(ptr, mystl::forward<Args>(args)...);
}

template <class T>
void allocator<T>::destroy(T* ptr)
{
  mystl::destroy(ptr);
}

template <class T>
void allocator<T>::destroy(T* first, T* last)
{
  mystl::destroy(first, last);
}

} // namespace mystl
#endif // !MYTINYSTL_ALLOCATOR_H_

