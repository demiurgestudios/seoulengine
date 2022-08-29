/**
 * \file Allocator.h
 * \brief Utility for memory allocation, used by SeoulEngine
 * containers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "Prereqs.h"
#include "SeoulMath.h"
#include <memory>

namespace Seoul
{

template <typename T>
class Allocator SEOUL_SEALED
{
public:
	static inline T* Allocate(size_t zNumberOfTypeT, MemoryBudgets::Type eType)
	{
		return (T*)MemoryManager::AllocateAligned(
			sizeof(T) * zNumberOfTypeT,
			eType,
			SEOUL_ALIGN_OF(T));
	}

	static inline T* ClearMemory(T* pDestination, size_t zNumberOfTypeT)
	{
		return (T*)memset(pDestination, 0, zNumberOfTypeT * sizeof(T));
	}

	static inline void Deallocate(T*& rp)
	{
		T* p = rp;
		rp = nullptr;
		MemoryManager::Deallocate(p);
	}

	static inline T* MemCpy(T* pDestination, T const* pSource, size_t zNumberOfTypeT)
	{
		return (T*)memcpy(pDestination, pSource, zNumberOfTypeT * sizeof(T));
	}

	static inline T* MemMove(T* pDestination, T const* pSource, size_t zNumberOfTypeT)
	{
		return (T*)memmove(pDestination, pSource, zNumberOfTypeT * sizeof(T));
	}

	static inline T* MemSet(T* pDestination, Int iValue, size_t zNumberOfTypeT)
	{
		return (T*)memset(pDestination, iValue, zNumberOfTypeT * sizeof(T));
	}

	static inline T* Reallocate(T* p, size_t zNumberOfTypeT, MemoryBudgets::Type eType)
	{
		return (T*)MemoryManager::ReallocateAligned(
			p,
			sizeof(T) * zNumberOfTypeT,
			SEOUL_ALIGN_OF(T),
			eType);
	}

private:
	// Not constructable, static class.
	Allocator();
	~Allocator();

	SEOUL_DISABLE_COPY(Allocator);
}; // class Allocator

template <typename T, int MEMORY_BUDGETS>
class StdContainerAllocator
{
public:
	typedef T         value_type;
	typedef T*        pointer;
	typedef const T*  const_pointer;
	typedef T&        reference;
	typedef const T&  const_reference;
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;

	template <typename U>
	struct rebind
	{
		typedef StdContainerAllocator<U, MEMORY_BUDGETS> other;
	};

	StdContainerAllocator() {}
	StdContainerAllocator(const StdContainerAllocator&) {}
	template <typename U> StdContainerAllocator(const StdContainerAllocator<U, MEMORY_BUDGETS>&) {}

	pointer address(reference x) const { return &x; }
	const_pointer address(const_reference x) const { return &x; }

	pointer allocate(size_type n, const void* hint = nullptr)
	{
		return (pointer)MemoryManager::AllocateAligned(
			sizeof(T) * n,
			(MemoryBudgets::Type)MEMORY_BUDGETS,
			SEOUL_ALIGN_OF(T));
	}

	void construct(pointer p, const_reference val)
	{
		new ((void*)p) T(val);
	}

	void deallocate(pointer p, size_type /*n*/)
	{
		MemoryManager::Deallocate(p);
	}

	void destroy(pointer p)
	{
		((T*)p)->~T();
	}

	bool operator==(const StdContainerAllocator&) const { return true; }
	bool operator!=(const StdContainerAllocator&) const { return false; }

	size_type max_size() const
	{
		return (size_type)IntMax;
	}
}; // class StdContainerAllocator

} // namespace Seoul

#endif // include guard
