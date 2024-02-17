#ifndef MYTINYSTL_ITERATOR_H_
#define MYTINYSTL_ITERATOR_H_

// 这个头文件用于迭代器设计，包含了一些模板结构体与全局函数，

#include <cstddef>

#include "type_traits.h"

namespace mystl
{

// 五种迭代器类型
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};//前向迭代器
struct bidirectional_iterator_tag : public forward_iterator_tag {};//双向迭代器
struct random_access_iterator_tag : public bidirectional_iterator_tag {};//随机迭代器

// iterator 模板
// ptrdiff_t表示指针之间的差异，通常用来表示数组元素之间的距离，在Win64中是long long,在其他平台是int
template <class Category, class T, class Distance = ptrdiff_t,
  class Pointer = T*, class Reference = T&>
struct iterator
{
  typedef Category                             iterator_category;//类别
  typedef T                                    value_type;
  typedef Pointer                              pointer;
  typedef Reference                            reference;
  typedef Distance                             difference_type;
};

// iterator traits 迭代器特性
// 使用SFINAE（Substitution Failure Is Not An Error）进行模板推导

// 判断是否有iterator_category属性
template <class T>
struct has_iterator_cat
{
private:
  struct two { char a; char b; };
  template <class U> static two test(...);
  template <class U> static char test(typename U::iterator_category* = 0);
public:
  static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <class Iterator, bool>
struct iterator_traits_impl {};

// 这里的true用与模板的判断选择
// 这个结构体实现了复用，将不同迭代器的类型统一定义名称iterator_category,这让后面模板判断出类型后可以得到对应迭代器的实际类型
template <class Iterator>
struct iterator_traits_impl<Iterator, true> 
{
  typedef typename Iterator::iterator_category iterator_category;
  typedef typename Iterator::value_type        value_type;
  typedef typename Iterator::pointer           pointer;
  typedef typename Iterator::reference         reference;
  typedef typename Iterator::difference_type   difference_type;
};

template <class Iterator, bool>
struct iterator_traits_helper {};

template <class Iterator>
struct iterator_traits_helper<Iterator, true> //这里的true用与模板的判断选择
  : public iterator_traits_impl<Iterator,
  std::is_convertible<typename Iterator::iterator_category, input_iterator_tag>::value ||
  std::is_convertible<typename Iterator::iterator_category, output_iterator_tag>::value>
{
};

// 萃取迭代器的特性 使用模板参数，层层嵌套
template <class Iterator>
struct iterator_traits 
  : public iterator_traits_helper<Iterator, has_iterator_cat<Iterator>::value> {};

// 针对原生指针的偏特化版本
template <class T>
struct iterator_traits<T*>
{
  typedef random_access_iterator_tag           iterator_category;
  typedef T                                    value_type;
  typedef T*                                   pointer;
  typedef T&                                   reference;
  typedef ptrdiff_t                            difference_type;
};

// 针对const的特化指针版本
template <class T>
struct iterator_traits<const T*>
{
  typedef random_access_iterator_tag           iterator_category;
  typedef T                                    value_type;
  typedef const T*                             pointer;
  typedef const T&                             reference;
  typedef ptrdiff_t                            difference_type;
};

// 一个用于判断两个迭代器的关系，T是否等于或者向下兼容U类型
// T是迭代器类型，U也是迭代器类型
// 第二是判断T的类型是否能隐式转换(向下兼容)为U类型
// 根据判断结果进行选择继承哪个父类，也就是自己的value值是true还是false
template <class T, class U, bool = has_iterator_cat<iterator_traits<T>>::value>
struct has_iterator_cat_of
  : public m_bool_constant<std::is_convertible<
  typename iterator_traits<T>::iterator_category, U>::value>
{
};

// 萃取某种迭代器
template <class T, class U>   //如果T不是迭代器类型，模板推导到这一步就结束了
struct has_iterator_cat_of<T, U, false> : public m_false_type {};

// 迭代器类型的判断
template <class Iter>
struct is_input_iterator : public has_iterator_cat_of<Iter, input_iterator_tag> {};

template <class Iter>
struct is_output_iterator : public has_iterator_cat_of<Iter, output_iterator_tag> {};

template <class Iter>
struct is_forward_iterator : public has_iterator_cat_of<Iter, forward_iterator_tag> {};

template <class Iter>
struct is_bidirectional_iterator : public has_iterator_cat_of<Iter, bidirectional_iterator_tag> {};

template <class Iter>
struct is_random_access_iterator : public has_iterator_cat_of<Iter, random_access_iterator_tag> {};

// input和output迭代器是所有迭代器的基础
template <class Iterator>
struct is_iterator :
  public m_bool_constant<is_input_iterator<Iterator>::value ||
    is_output_iterator<Iterator>::value>
{
};


// 
// 萃取某个迭代器的 category
// 模板参数传一个具体迭代器的类型，返回这迭代器种类对应的实例，类型对应于此头文件最开始定义的五种迭代器类型
template <class Iterator>
typename iterator_traits<Iterator>::iterator_category
iterator_category(const Iterator&)
{
  typedef typename iterator_traits<Iterator>::iterator_category Category;
  return Category();//类的默认构造函数
}

// 萃取某个迭代器的 distance_type ,目的是获取类型
template <class Iterator>
typename iterator_traits<Iterator>::difference_type*
distance_type(const Iterator&)
{
  return static_cast<typename iterator_traits<Iterator>::difference_type*>(0);
}

// 萃取某个迭代器的 value_type ,目的是获取类型
template <class Iterator>
typename iterator_traits<Iterator>::value_type*
value_type(const Iterator&)
{
  return static_cast<typename iterator_traits<Iterator>::value_type*>(0);
}

// 以下函数用于计算迭代器间的距离

// distance 的 input_iterator_tag 的版本
template <class InputIterator>
typename iterator_traits<InputIterator>::difference_type
distance_dispatch(InputIterator first, InputIterator last, input_iterator_tag)//占位参数
{
  typename iterator_traits<InputIterator>::difference_type n = 0;
  while (first != last)
  {
    ++first;
    ++n;
  }
  return n;
}

// distance 的 random_access_iterator_tag 的版本
template <class RandomIter>
typename iterator_traits<RandomIter>::difference_type
distance_dispatch(RandomIter first, RandomIter last,
                  random_access_iterator_tag)
{
  return last - first;
}

template <class InputIterator>
typename iterator_traits<InputIterator>::difference_type
distance(InputIterator first, InputIterator last)
{
  return distance_dispatch(first, last, iterator_category(first));
}

// 以下函数用于让迭代器前进 n 个距离

// advance 的 input_iterator_tag 的版本
template <class InputIterator, class Distance>
void advance_dispatch(InputIterator& i, Distance n, input_iterator_tag)
{
  while (n--) 
    ++i;
}

// advance 的 bidirectional_iterator_tag 的版本
template <class BidirectionalIterator, class Distance>
void advance_dispatch(BidirectionalIterator& i, Distance n, bidirectional_iterator_tag)
{
  if (n >= 0)
    while (n--)  ++i;
  else
    while (n++)  --i;
}

// advance 的 random_access_iterator_tag 的版本
template <class RandomIter, class Distance>
void advance_dispatch(RandomIter& i, Distance n, random_access_iterator_tag)
{
  i += n;
}

template <class InputIterator, class Distance>
void advance(InputIterator& i, Distance n)
{
  advance_dispatch(i, n, iterator_category(i));
}

// 这里的设计思想是将模板函数的最后一个参数设置为类型标志位，通过占位参数的方式实现函数的重载
// 最后将函数封装，通过参数判断调用哪个版本


/*****************************************************************************************/

// 模板类 : reverse_iterator
// 代表反向迭代器，使前进为后退，后退为前进
template <class Iterator>
class reverse_iterator
{
private:
  Iterator current;  // 记录对应的正向迭代器

public:
  // 反向迭代器的五种相应型别
  typedef typename iterator_traits<Iterator>::iterator_category iterator_category;
  typedef typename iterator_traits<Iterator>::value_type        value_type;
  typedef typename iterator_traits<Iterator>::difference_type   difference_type;
  typedef typename iterator_traits<Iterator>::pointer           pointer;
  typedef typename iterator_traits<Iterator>::reference         reference;

  typedef Iterator                                              iterator_type;
  typedef reverse_iterator<Iterator>                            self;

public:
  // 构造函数
  reverse_iterator() {}
  explicit reverse_iterator(iterator_type i) :current(i) {} // 这里为什么不传入引用？
  reverse_iterator(const self& rhs) :current(rhs.current) {}

public:
  // 取出对应的正向迭代器
  iterator_type base() const 
  { return current; }

  // 重载操作符
  reference operator*() const
  { // 实际对应正向迭代器的前一个位置，注意是前一个位置，因为end一般是不存放值的
    auto tmp = current;
    return *--tmp;
  }

  pointer operator->() const
  {
    return &(operator*());
  }

  // 前进(++)变为后退(--)
  self& operator++()//左加
  {
    --current;
    return *this;
  }
  self operator++(int)//右加
  {
    self tmp = *this;
    --current;
    return tmp;
  }
  // 后退(--)变为前进(++)
  self& operator--()//左减
  {
    ++current;
    return *this;
  }
  self operator--(int)//右减
  {
    self tmp = *this;
    ++current;
    return tmp;
  }

  self& operator+=(difference_type n)
  {
    current -= n;
    return *this;
  }
  self operator+(difference_type n) const
  {
    return self(current - n);
  }
  self& operator-=(difference_type n)
  {
    current += n;
    return *this;
  }
  self operator-(difference_type n) const
  {
    return self(current + n);
  }

  reference operator[](difference_type n) const
  {
    return *(*this + n);
  }
};

// 重载 operator-
template <class Iterator>
typename reverse_iterator<Iterator>::difference_type
operator-(const reverse_iterator<Iterator>& lhs,
          const reverse_iterator<Iterator>& rhs)
{
  return rhs.base() - lhs.base();
}

// 重载比较操作符
template <class Iterator>
bool operator==(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<Iterator>& rhs)
{
  return lhs.base() == rhs.base();
}

template <class Iterator>
bool operator<(const reverse_iterator<Iterator>& lhs,
  const reverse_iterator<Iterator>& rhs)
{
  return rhs.base() < lhs.base();
}

template <class Iterator>
bool operator!=(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<Iterator>& rhs)
{
  return !(lhs == rhs);
}

template <class Iterator>
bool operator>(const reverse_iterator<Iterator>& lhs,
               const reverse_iterator<Iterator>& rhs)
{
  return rhs < lhs;
}

template <class Iterator>
bool operator<=(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<Iterator>& rhs)
{
  return !(rhs < lhs);
}

template <class Iterator>
bool operator>=(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<Iterator>& rhs)
{
  return !(lhs < rhs);
}

} // namespace mystl

#endif // !MYTINYSTL_ITERATOR_H_

