#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//被动调用，哪个线程来了之后，需要内存就调用这个接口
static inline void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)//超过一个最大值 64k，认为是大对象，直接从PageCache中获取
	{
		//return malloc(size);
		Span* span = PageCache::GetInstence()->AllocBigPageObj(size);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		//变量tlslist用来申请工具
		if (tlslist == nullptr)//第一次来，自己创建，后面来的，就可以直接使用当前创建好的内存池
		{
			tlslist = new ThreadCache;
		}
		return tlslist->Allocate(size);
	}
}



static inline void ConcurrentFree(void* ptr)//最后释放
{
	Span* span = PageCache::GetInstence()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;
	if (size > MAX_BYTES)
	{
		//free(ptr);
		PageCache::GetInstence()->FreeBigPageObj(ptr, span);
	}
	else
	{
		tlslist->Deallocate(ptr, size);
	}
}