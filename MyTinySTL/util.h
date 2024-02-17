#ifndef MYTINYSTL_UTIL_H_
#define MYTINYSTL_UTIL_H_

// 这个文件包含一些通用工具，包括 move, forward, swap 等函数，以及 pair 等 

#include <cstddef>

#include "type_traits.h"

namespace mystl
{

// move

template <class T> //typename关键字用来显示指出某个名称是类型而不是变量
typename std::remove_reference<T>::type&& move(T&& arg) noexcept //std::remove_reference<T>::type用来获取T类型的非引用版本，因为其特化了&和&&
{
  return static_cast<typename std::remove_reference<T>::type&&>(arg);//强制转换为右值类型，减少拷贝次数，可以这么理解move函数的作用，如果传入的是左值，则将其转化为右值并返回，如果是右值，则延迟它的生命周期
}

// forward 与move语义不同在于forward一个重要的作用是传递参数原本的类型，例如const和右值属性而且forward函数一般用在模板函数内部
// 这里是为了显示的区分左值引用还是右值引用而特化了两个模板函数，但是根据C++11的引用折叠规则，可以只写一个(参数是&&)
template <class T>
T&& forward(typename std::remove_reference<T>::type& arg) noexcept
{
  return static_cast<T&&>(arg);
}

template <class T>
T&& forward(typename std::remove_reference<T>::type&& arg) noexcept
{
  static_assert(!std::is_lvalue_reference<T>::value, "bad forward");
  return static_cast<T&&>(arg);
}

// swap

template <class Tp>
void swap(Tp& lhs, Tp& rhs)
{
  auto tmp(mystl::move(lhs));//在swap函数中使用move语义，减少隐式复制操作
  lhs = mystl::move(rhs);
  rhs = mystl::move(tmp);
}

template <class ForwardIter1, class ForwardIter2>
ForwardIter2 swap_range(ForwardIter1 first1, ForwardIter1 last1, ForwardIter2 first2)//等长的连续元素互换
{
  for (; first1 != last1; ++first1, (void) ++first2)
    mystl::swap(*first1, *first2);
  return first2;
}

template <class Tp, size_t N>
void swap(Tp(&a)[N], Tp(&b)[N])//传入两个数组的引用
{
  mystl::swap_range(a, a + N, b);
}

// --------------------------------------------------------------------------------------
// pair

// 结构体模板 : pair
// 两个模板参数分别表示两个数据的类型
// 用 first 和 second 来分别取出第一个数据和第二个数据
template <class Ty1, class Ty2>
struct pair
{
  typedef Ty1    first_type;
  typedef Ty2    second_type;

  first_type first;    // 保存第一个数据
  second_type second;  // 保存第二个数据


  /*
  1 使用SFINAE（Substitution Failure Is Not An Error）匹配错误不是失败 技术，它通过模板参数的匹配来实现对函数的启用或禁用
  2 std::enable_if<T>用于更具条件是否启用模板参数
  3 std::is_default_constructible<T>用来判断T是否是可默认构造的类型
  4 当判断Other1和Other2不满足默认构造时，enable_if类将不会有成员type，进而导致模板参数推导失败，进一步的情况是编译器会将此函数从候选函数列表中移除
  5 这是一种在模板编程中根据条件进行编译时选择的常见技巧。通过 SFINAE，可以在编译时排除那些不符合条件的代码分支
  6 constexpr修饰该函数，因此该函数可以在编译时执行
  7 使用初始化列表
  */

  // 通过一个匿名模板类型进行一个条件判断
  // default constructiable
  template <class Other1 = Ty1, class Other2 = Ty2,
    typename = typename std::enable_if<
    std::is_default_constructible<Other1>::value &&
    std::is_default_constructible<Other2>::value, void>::type>
    constexpr pair()
    : first(), second()
  {
  }

  /*
  1 std::is_copy_constructible<T>用于判断T是否可以复制构造
  2 std::is_convertible<T1,T2>用于判断T1类型是否能够转换为T2类型
  */
  // implicit constructiable for this type 隐式构造函数/复制构造函数
  template <class U1 = Ty1, class U2 = Ty2,
    typename std::enable_if<
    std::is_copy_constructible<U1>::value &&
    std::is_copy_constructible<U2>::value &&
    std::is_convertible<const U1&, Ty1>::value &&
    std::is_convertible<const U2&, Ty2>::value, int>::type = 0>
    constexpr pair(const Ty1& a, const Ty2& b)
    : first(a), second(b)
  {
  }

  /*
  对于const Un&类型不能强制转换为Tyn的情况，应当禁止隐式拷贝，因此该构造函数使用explicit防止出现隐式构造而产生错误
  */
  // explicit constructible for this type 
  template <class U1 = Ty1, class U2 = Ty2,
    typename std::enable_if<
    std::is_copy_constructible<U1>::value &&
    std::is_copy_constructible<U2>::value &&
    (!std::is_convertible<const U1&, Ty1>::value ||
     !std::is_convertible<const U2&, Ty2>::value), int>::type = 0>
    explicit constexpr pair(const Ty1& a, const Ty2& b)
    : first(a), second(b)
  {
  }
  
  /*
  使用default关键字，意味着这两个函数时缺省函数，使用编译器提供的默认版本函数
  */
  pair(const pair& rhs) = default;
  pair(pair&& rhs) = default;

  /*
  1 移动构造函数，使用移动语义
  2 std::is_constructible<Tyn,Othern>用来判断Tyn类型是否能够通过Othern类型构造
  3 使用forward函数实现右值的传递
  */
  // implicit constructiable for other type
  template <class Other1, class Other2,
    typename std::enable_if<
    std::is_constructible<Ty1, Other1>::value &&
    std::is_constructible<Ty2, Other2>::value &&
    std::is_convertible<Other1&&, Ty1>::value &&
    std::is_convertible<Other2&&, Ty2>::value, int>::type = 0>
    constexpr pair(Other1&& a, Other2&& b)
    : first(mystl::forward<Other1>(a)),
    second(mystl::forward<Other2>(b))
  {
  }

  /*
  显式移动构造函数
  */
  // explicit constructiable for other type
  template <class Other1, class Other2,
    typename std::enable_if<
    std::is_constructible<Ty1, Other1>::value &&
    std::is_constructible<Ty2, Other2>::value &&
    (!std::is_convertible<Other1, Ty1>::value ||
     !std::is_convertible<Other2, Ty2>::value), int>::type = 0>
    explicit constexpr pair(Other1&& a, Other2&& b)
    : first(mystl::forward<Other1>(a)),
    second(mystl::forward<Other2>(b))
  {
  }

  /*
  复制构造函数，使用另一个pair类型变量构造
  */
  // implicit constructiable for other pair
  template <class Other1, class Other2,
    typename std::enable_if<
    std::is_constructible<Ty1, const Other1&>::value &&
    std::is_constructible<Ty2, const Other2&>::value &&
    std::is_convertible<const Other1&, Ty1>::value &&
    std::is_convertible<const Other2&, Ty2>::value, int>::type = 0>
    constexpr pair(const pair<Other1, Other2>& other)
    : first(other.first),
    second(other.second)
  {
  }

  /*
  特殊类型的构造必须显式构造
  */
  // explicit constructiable for other pair
  template <class Other1, class Other2,
    typename std::enable_if<
    std::is_constructible<Ty1, const Other1&>::value &&
    std::is_constructible<Ty2, const Other2&>::value &&
    (!std::is_convertible<const Other1&, Ty1>::value ||
     !std::is_convertible<const Other2&, Ty2>::value), int>::type = 0>
    explicit constexpr pair(const pair<Other1, Other2>& other)
    : first(other.first),
    second(other.second)
  {
  }

  // implicit constructiable for other pair
  template <class Other1, class Other2,
    typename std::enable_if<
    std::is_constructible<Ty1, Other1>::value &&
    std::is_constructible<Ty2, Other2>::value &&
    std::is_convertible<Other1, Ty1>::value &&
    std::is_convertible<Other2, Ty2>::value, int>::type = 0>
    constexpr pair(pair<Other1, Other2>&& other)
    : first(mystl::forward<Other1>(other.first)),
    second(mystl::forward<Other2>(other.second))
  {
  }

  // explicit constructiable for other pair
  template <class Other1, class Other2,
    typename std::enable_if<
    std::is_constructible<Ty1, Other1>::value &&
    std::is_constructible<Ty2, Other2>::value &&
    (!std::is_convertible<Other1, Ty1>::value ||
     !std::is_convertible<Other2, Ty2>::value), int>::type = 0>
    explicit constexpr pair(pair<Other1, Other2>&& other)
    : first(mystl::forward<Other1>(other.first)),
    second(mystl::forward<Other2>(other.second))
  {
  }


  /*
  以上是pair构造函数的全部内容，主要可以分为三大类，一类是使用first和second来进行构造，第二种是通过另一个pair对象来构造，第三种是默认构造
  每一类构造又分为隐式构造和显式构造(对应那些不允许显式构造的first或者second的类型)，同时又存在复制构造和移动构造两种方式。
  *****************************************************************************************************************************
  */


  // copy assign for this pair 为自身相同类型的pair提供的拷贝赋值运算符重载
  pair& operator=(const pair& rhs)
  {
    if (this != &rhs)
    {
      first = rhs.first;
      second = rhs.second;
    }
    return *this;
  }

  // move assign for this pair 为自身相同的类型的pair提供的移动赋值运算符重载
  pair& operator=(pair&& rhs)
  {
    if (this != &rhs)
    {
      first = mystl::move(rhs.first);
      second = mystl::move(rhs.second);
    }
    return *this;
  }

  // copy assign for other pair 为自身不同类型的pair提供的拷贝赋值运算符重载
  template <class Other1, class Other2>
  pair& operator=(const pair<Other1, Other2>& other)
  {
    first = other.first;
    second = other.second;
    return *this;
  }

  // move assign for other pair 为自身不同的类型的pair提供的移动赋值运算符重载
  template <class Other1, class Other2>
  pair& operator=(pair<Other1, Other2>&& other)
  {
    first = mystl::forward<Other1>(other.first);
    second = mystl::forward<Other2>(other.second);
    return *this;
  }

  //使用默认的析构函数，会调用first，second自身的析构函数
  ~pair() = default;
  
  //交换函数
  void swap(pair& other)
  {
    if (this != &other)
    {
      mystl::swap(first, other.first);
      mystl::swap(second, other.second);
    }
  }

};

// 重载比较操作符 
template <class Ty1, class Ty2>
bool operator==(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs)
{
  return lhs.first == rhs.first && lhs.second == rhs.second;
}

//pair类型比较大小的逻辑是首先比较第一个元素，如果第一个元素相同则继续比价第二个元素
template <class Ty1, class Ty2>
bool operator<(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs)
{
  return lhs.first < rhs.first || (lhs.first == rhs.first && lhs.second < rhs.second);
}

template <class Ty1, class Ty2>
bool operator!=(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs)
{
  return !(lhs == rhs);
}

//调用重载的>运算符
template <class Ty1, class Ty2>
bool operator>(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs)
{
  return rhs < lhs;
}

template <class Ty1, class Ty2>
bool operator<=(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs)
{
  return !(rhs < lhs);
}

template <class Ty1, class Ty2>
bool operator>=(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs)
{
  return !(lhs < rhs);
}

// 重载 mystl 的 swap
template <class Ty1, class Ty2>
void swap(pair<Ty1, Ty2>& lhs, pair<Ty1, Ty2>& rhs)
{
  lhs.swap(rhs);
}

// 全局函数，让两个数据成为一个 pair 参数只需要一个右值版本即可，因为左值用非左值引用类型传进来也是一个右值
template <class Ty1, class Ty2>
pair<Ty1, Ty2> make_pair(Ty1&& first, Ty2&& second)
{
  return pair<Ty1, Ty2>(mystl::forward<Ty1>(first), mystl::forward<Ty2>(second));
}

}

#endif // !MYTINYSTL_UTIL_H_

