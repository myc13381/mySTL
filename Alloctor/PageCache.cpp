#include "PageCache.h"

PageCache PageCache::_inst;


//大对象申请，直接从系统
Span* PageCache::AllocBigPageObj(size_t size)
{
	assert(size > MAX_BYTES);//只有申请64K以上内存时才需要调用此函数

	size = SizeClass::_Roundup(size, PAGE_SHIFT); //对齐
	size_t npage = size >> PAGE_SHIFT;
	if (npage < NPAGES)
	{
		Span* span = NewSpan(npage);
		span->_objsize = size;
		span->_usecount = 1;
		return span;
	}
	else//超过128页，向系统申请
	{
#ifdef _WIN32
		void* ptr = VirtualAlloc(0, npage << PAGE_SHIFT,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else 
		void* ptr = malloc(npage << PAGE_SHIFT);
#endif

		if (ptr == nullptr)
			throw std::bad_alloc();

		Span* span = new Span;
		span->_npage = npage;
		span->_pageid = (PageID)ptr >> PAGE_SHIFT;
		span->_objsize = npage << PAGE_SHIFT;

		_idspanmap[span->_pageid] = span;

		return span;
	}
}

void PageCache::FreeBigPageObj(void* ptr, Span* span)
{
	size_t npage = span->_objsize >> PAGE_SHIFT;
	if (npage < NPAGES) //相当于还是小于128页
	{
		ReleaseSpanToPageCache(span);
	}
	else
	{
		_idspanmap.erase(npage);
		//void* ptr = (void*)(span->_pageid << PAGE_SHIFT);//是否可以这样做然后少传递一个参数
		delete span;
#ifdef _WIN32
		VirtualFree(ptr, 0, MEM_RELEASE);
#else 
		free(ptr);
#endif
	}
}

Span* PageCache::NewSpan(size_t n)
{
	// 加锁，防止多个线程同时到PageCache中申请span
	// 这里必须是给全局加锁，不能单独的给每个桶加锁
	// 如果对应桶没有span,是需要向系统申请的
	// 可能存在多个线程同时向系统申请内存的可能
	std::unique_lock<std::mutex> lock(_mutex);
	return _NewSpan(n);
}



Span* PageCache::_NewSpan(size_t n)
{
	assert(n < NPAGES);
	if (!_spanlist[n].Empty())
	{
		Span* span =_spanlist[n].PopFront();
		span->_usecount = 1;
		return span;
	}
		

	for (size_t i = n + 1; i < NPAGES; ++i)
	{
		if (!_spanlist[i].Empty())
		{
			//大内存对象拆分
			Span* span = _spanlist[i].PopFront();
			Span* splist = new Span;

			splist->_pageid = span->_pageid;
			splist->_npage = n;
			splist->_objsize = splist->_npage << PAGE_SHIFT;
			splist->_usecount = 1;//一次使用

			span->_pageid = span->_pageid + n;
			span->_npage = span->_npage - n;
			span->_objsize = span->_npage << PAGE_SHIFT;


			for (size_t j = 0; j < n; ++j)
				_idspanmap[splist->_pageid + j] = splist;

			//_spanlist[splist->_npage].PushFront(splist);

			_spanlist[span->_npage].PushFront(span);
			return splist;
		}
	}

	Span* span = new Span;

	// 到这里说明SpanList中没有合适的span,只能向系统申请128页的内存
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, (NPAGES - 1)*(1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//  brk
	void* ptr=malloc((NPAGES - 1)*(1 << PAGE_SHIFT));
#endif


	span->_pageid = (PageID)ptr >> PAGE_SHIFT;
	span->_npage = NPAGES - 1;

	for (size_t i = 0; i < span->_npage; ++i)
		_idspanmap[span->_pageid + i] = span;

	_spanlist[span->_npage].PushFront(span);  //Span->_next  Span->_prev 
	return _NewSpan(n);
}

// 获取从对象到span的映射
Span* PageCache::MapObjectToSpan(void* obj)
{
	//计算页号
	PageID id = (PageID)obj >> PAGE_SHIFT;
	auto it = _idspanmap.find(id);
	if (it != _idspanmap.end())
	{
		return it->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* cur)
{
	// 必须上全局锁,可能多个线程一起从ThreadCache中归还数据
	std::unique_lock<std::mutex> lock(_mutex);
	cur->_objsize = 0;
	cur->_usecount = 0;

	// 向前合并
	while (1)
	{
		PageID curid = cur->_pageid;
		PageID previd = curid - 1;
		auto it = _idspanmap.find(previd);

		// 没有找到
		if (it == _idspanmap.end())
			break;

		// 前一个span不空闲
		if (it->second->_usecount != 0)
			break;

		Span* prev = it->second;

		//超过128页则不合并
		if (cur->_npage + prev->_npage > NPAGES - 1)
			break;

		// 先把prev从链表中移除
		_spanlist[prev->_npage].Erase(prev);

		// 合并
		prev->_npage += cur->_npage;
		//修正id->span的映射关系
		for (PageID i = 0; i < cur->_npage; ++i)
		{
			_idspanmap[cur->_pageid + i] = prev;
		}
		delete cur;

		// 继续向前合并
		cur = prev;
	}


	//向后合并
	while (1)
	{
		////超过128页则不合并

		PageID curid = cur->_pageid;
		PageID nextid = curid + cur->_npage;
		//std::map<PageID, Span*>::iterator it = _idspanmap.find(nextid);
		auto it = _idspanmap.find(nextid);

		if (it == _idspanmap.end())
			break;

		if (it->second->_usecount != 0)
			break;

		Span* next = it->second;

		//超过128页则不合并
		if (cur->_npage + next->_npage >= NPAGES - 1)
			break;

		_spanlist[next->_npage].Erase(next);


		cur->_npage += next->_npage;
		//修正id->Span的映射关系
		for (PageID i = 0; i < next->_npage; ++i)
		{
			_idspanmap[next->_pageid + i] = cur;
		}

		delete next;
	}

	// 最后将合并好的span插入到span链中
	_spanlist[cur->_npage].PushFront(cur);
}

PageCache::~PageCache()
{

}