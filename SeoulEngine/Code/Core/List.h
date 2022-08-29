/**
 * \file List.h
 * \brief SeoulEngine equivalent to std::list<>. Identical to std::list<> API, except:
 * - CamelCase function and members names.
 * - additional Contains(), and ContainsFromBack(), Find(), FindFromBack().
 * - additional RemoveFirstInstance().
 * - empty() is called IsEmpty().
 * - size() is called GetSize().
 * - SizeType is always a UInt32.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	LIST_H
#define LIST_H

#include "Algorithms.h"
#include "Allocator.h"
#include <list>

namespace Seoul
{

template <typename T, int MEMORY_BUDGETS>
class List SEOUL_SEALED
{
public:
	typedef StdContainerAllocator<T, MEMORY_BUDGETS>     AllocatorType;
	typedef std::list<T, AllocatorType>                  StdListType;

	typedef typename StdListType::value_type             ValueType;
	typedef UInt32                                       SizeType;
	typedef typename StdListType::difference_type        DifferenceType;
	typedef typename StdListType::pointer                Pointer;
	typedef typename StdListType::const_pointer          ConstPointer;
	typedef typename StdListType::reference              Reference;
	typedef typename StdListType::const_reference        ConstReference;
	typedef typename StdListType::iterator               iterator; /* range-based for loop */
	typedef typename StdListType::iterator               Iterator;
	typedef typename StdListType::const_iterator         const_iterator; /* range-based for loop */
	typedef typename StdListType::const_iterator         ConstIterator;
	typedef typename StdListType::reverse_iterator       ReverseIterator;
	typedef typename StdListType::const_reverse_iterator ConstReverseIterator;

	               List()                                               : m_List() {}
	               List(const List& b)                                  : m_List(b.m_List) {}
	               template <int B_MEMORY_BUDGETS>
	               List(const List<T, B_MEMORY_BUDGETS>& b)             : m_List(b.Begin(), b.End()) {}
	explicit       List(SizeType n, ConstReference val = ValueType())   : m_List(n, val) {}

	void           Assign(SizeType n, ConstReference val = ValueType()) { m_List.assign(n, val); }
	               template <typename ITER>
	void           Assign(ITER begin, ITER end)                         { m_List.assign(begin, end); }

	Reference      Back()                                               { return m_List.back(); }
	ConstReference Back() const                                         { return m_List.back(); }

	iterator       begin() /* range-based for loop */                   { return Begin(); }
	const_iterator begin() const /* range-based for loop */             { return Begin(); }
	Iterator       Begin()                                              { return m_List.begin(); }
	ConstIterator  Begin() const                                        { return m_List.begin(); }

	void           Clear()                                              { m_List.clear(); }

	Bool           Contains(ConstReference val) const                   { return Seoul::Contains(Begin(), End(), val); }
	template       <typename U>
	Bool           Contains(const U& val) const                         { return Seoul::Contains(Begin(), End(), val); }

	Bool           ContainsFromBack(ConstReference val) const           { return Seoul::ContainsFromBack(Begin(), End(), val); }
	template       <typename U>
	Bool           ContainsFromBack(const U& val) const                 { return Seoul::ContainsFromBack(Begin(), End(), val); }

	iterator       end() /* range-based for loop */                     { return End(); }
	const_iterator end() const /* range-based for loop */               { return End(); }
	Iterator       End()                                                { return m_List.end(); }
	ConstIterator  End() const                                          { return m_List.end(); }

	Iterator       Erase(Iterator pos)                                  { return m_List.erase(pos); }
	Iterator       Erase(Iterator first, Iterator last)                 { return m_List.erase(first, last); }

	Iterator       Find(ConstReference val)                             { return Seoul::Find(Begin(), End(), val); }
	               template <typename U>
	Iterator       Find(const U& val)                                   { return Seoul::Find(Begin(), End(), val); }
	ConstIterator  Find(ConstReference val) const                       { return Seoul::Find(Begin(), End(), val); }
	               template <typename U>
	ConstIterator  Find(const U& val) const                             { return Seoul::Find(Begin(), End(), val); }

	Iterator       FindFromBack(ConstReference val)                     { return Seoul::FindFromBack(Begin(), End(), val); }
	               template <typename U>
	Iterator       FindFromBack(const U& val)                           { return Seoul::FindFromBack(Begin(), End(), val); }
	ConstIterator  FindFromBack(ConstReference val) const               { return Seoul::FindFromBack(Begin(), End(), val); }
	               template <typename U>
	ConstIterator  FindFromBack(const U& val) const                     { return Seoul::FindFromBack(Begin(), End(), val); }

	Reference      Front()                                              { return m_List.front(); }
	ConstReference Front() const                                        { return m_List.front(); }

	SizeType       GetSize() const                                      { return (SizeType)m_List.size(); }

	Iterator       Insert(Iterator pos, ConstReference val)             { return m_List.insert(pos, val); }

	Bool           IsEmpty() const                                      { return m_List.empty(); }

	List&          operator=(const List& b)                             { m_List = b.m_List; return *this; }
	               template <int B_MEMORY_BUDGETS>
	List&          operator=(const List<T, B_MEMORY_BUDGETS>& b)        { m_List.assign(b.Begin(), b.End()); return *this; }

	void           PopBack()                                            { m_List.pop_back(); }
	void           PopFront()                                           { m_List.pop_front(); }
	void           PushBack(ConstReference val)                         { m_List.push_back(val); }
	void           PushFront(ConstReference val)                        { m_List.push_front(val); }

	UInt32         Remove(ConstReference val);
	               template <typename U>
	UInt32         Remove(const U& val);
	               template <typename PRED>
	UInt32         RemoveIf(PRED pred);
	Bool           RemoveFirstInstance(ConstReference val);
	               template <typename U>
	Bool           RemoveFirstInstance(const U& val);

	void           Resize(SizeType n, ConstReference val = ValueType()) { m_List.resize(n, val); }

	void           Reverse()                                            { m_List.reverse(); }

	void           Sort()                                               { m_List.sort(); }
	               template <typename COMP>
	void           Sort(COMP comp)                                      { m_List.sort(comp); }

	void           Swap(List& b)                                        { m_List.swap(b.m_List); }

	ReverseIterator      RBegin()       { return m_List.rbegin(); }
	ConstReverseIterator RBegin() const { return m_List.rbegin(); }
	ReverseIterator      REnd()         { return m_List.rend(); }
	ConstReverseIterator REnd() const   { return m_List.rend(); }

private:
	StdListType m_List;
}; // class List

template <typename T, int MEMORY_BUDGETS>
inline UInt32 List<T, MEMORY_BUDGETS>::Remove(ConstReference val)
{
	UInt32 uReturn = 0u;

	auto const tmp(val); // Copy so val cannot change, due to const ref.
	auto i = Begin();
	for (; i != End(); )
	{
		if (*i == tmp)
		{
			i = Erase(i);
			++uReturn;
		}
		else
		{
			++i;
		}
	}

	return uReturn;
}

template <typename T, int MEMORY_BUDGETS>
template <typename U>
inline UInt32 List<T, MEMORY_BUDGETS>::Remove(const U& val)
{
	UInt32 uReturn = 0u;

	auto const tmp(val); // Copy so val cannot change, due to const ref.
	auto i = Begin();
	for (; i != End(); )
	{
		if (*i == tmp)
		{
			i = Erase(i);
			++uReturn;
		}
		else
		{
			++i;
		}
	}

	return uReturn;
}

template <typename T, int MEMORY_BUDGETS>
template <typename PRED>
inline UInt32 List<T, MEMORY_BUDGETS>::RemoveIf(PRED pred)
{
	UInt32 uReturn = 0u;

	auto i = Begin();
	for (; i != End(); )
	{
		if (pred(*i))
		{
			i = Erase(i);
			++uReturn;
		}
		else
		{
			++i;
		}
	}

	return uReturn;
}

/**
 * No equivalent in std. Removes the first instance of an element which matches
 * the given value if it exists. Returns an iterator to the new end.
 * TODO: add unit tests
 * 
 * @return True if the element was found and removed, false otherwise.
 */
template <typename T, int MEMORY_BUDGETS>
inline Bool List<T, MEMORY_BUDGETS>::RemoveFirstInstance(ConstReference val)
{
	auto element = Find(val);
	if (element != End())
	{
		Erase(element);
		return true;
	}
	return false;
}

/**
 * No equivalent in std. Removes the first instance of an element which matches
 * the given value if it exists. Returns an iterator to the new end.
 * TODO: add unit tests
 *
 * @return True if the element was found and removed, false otherwise.
 */
template <typename T, int MEMORY_BUDGETS>
template <typename U>
inline Bool List<T, MEMORY_BUDGETS>::RemoveFirstInstance(const U& val)
{
	auto element = Find(val);
	if (element != End())
	{
		Erase(element);
		return true;
	}
	return false;
}

/** @return True if VectorA == VectorB, false otherwise. */
template <typename TA, int MEMORY_BUDGETS_A, typename TB, int MEMORY_BUDGETS_B>
inline Bool operator==(const List<TA, MEMORY_BUDGETS_A>& vA, const List<TB, MEMORY_BUDGETS_B>& vB)
{
	if (vA.GetSize() != vB.GetSize())
	{
		return false;
	}

	auto const iBeginA = vA.Begin();
	auto const iEndA = vA.End();
	auto const iBeginB = vB.Begin();
	for (auto iA = iBeginA, iB = iBeginB; iEndA != iA; ++iA, ++iB)
	{
		if (*iA != *iB)
		{
			return false;
		}
	}

	return true;
}

/** @return True if VectorA == VectorB, false otherwise. */
template <typename TA, int MEMORY_BUDGETS_A, typename TB, int MEMORY_BUDGETS_B>
inline Bool operator!=(const List<TA, MEMORY_BUDGETS_A>& vA, const List<TB, MEMORY_BUDGETS_B>& vB)
{
	return !(vA == vB);
}

/**
 * Deletes heap allocated objects referenced in the list rList, and then clears
 * the List.
 */
template <typename T, int MEMORY_BUDGETS>
inline void SafeDeleteList(List<T*, MEMORY_BUDGETS>& rList)
{
	while (!rList.IsEmpty())
	{
		SEOUL_DELETE rList.Front();
		rList.PopFront();
	}
}

/**
 * Equivalent to std::swap(). Override specifically for List<>.
 */
template <typename T, int MEMORY_BUDGETS>
inline void Swap(List<T, MEMORY_BUDGETS>& a, List<T, MEMORY_BUDGETS>& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
