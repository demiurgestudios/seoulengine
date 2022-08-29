/**
 * \file Queue.h
 * \brief SeoulEngine equivalent to std::queue<>. Identical to std::queue<> API, except:
 * - additional Begin()/End() iterator support.
 * - CamelCase function and members names.
 * - addition of Begin()/End() and associated iterators.
 * - addition of Clear().
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
#ifndef	QUEUE_H
#define QUEUE_H

#include "List.h"

namespace Seoul
{

template <typename T, int MEMORY_BUDGETS>
class Queue SEOUL_SEALED
{
public:
	typedef List<T, MEMORY_BUDGETS>                   ContainerType;

	typedef typename ContainerType::ValueType         ValueType;
	typedef typename ContainerType::SizeType          SizeType;
	typedef typename ContainerType::DifferenceType    DifferenceType;
	typedef typename ContainerType::Pointer           Pointer;
	typedef typename ContainerType::ConstPointer      ConstPointer;
	typedef typename ContainerType::Reference         Reference;
	typedef typename ContainerType::ConstReference    ConstReference;
	typedef typename ContainerType::Iterator          Iterator;
	typedef typename ContainerType::ConstIterator     ConstIterator;

	Queue()                       : m_List() {}

	Reference      Back()         { return m_List.Back(); }
	ConstReference Back() const   { return m_List.Back(); }

	Iterator      Begin()         { return m_List.Begin(); }
	ConstIterator Begin() const   { return m_List.Begin(); }

	void          Clear()         { m_List.Clear(); }

	Iterator      End()           { return m_List.End(); }
	ConstIterator End() const     { return m_List.End(); }

	Reference      Front()        { return m_List.Front(); }
	ConstReference Front() const  { return m_List.Front(); }

	const ContainerType& GetList() const { return m_List; }
	ContainerType&       GetList()       { return m_List; }

	SizeType GetSize() const      { return m_List.GetSize(); }

	Bool IsEmpty() const          { return m_List.IsEmpty(); }

	void Pop()                    { m_List.PopFront(); }
	void Push(ConstReference val) { m_List.PushBack(val); }

	void Swap(Queue& b)           { m_List.Swap(b.m_List); }

private:
	ContainerType m_List;
}; // class Queue

/**
 * Deletes heap allocated objects referenced in the list rQueue, and then clears
 * the Queue.
 */
template <typename T, int MEMORY_BUDGETS>
inline void SafeDeleteQueue(Queue<T*, MEMORY_BUDGETS>& rQueue)
{
	while (!rQueue.IsEmpty())
	{
		SEOUL_DELETE rQueue.Front();
		rQueue.Pop();
	}
}

/**
 * Equivalent to std::swap(). Override specifically for Queue<>.
 */
template <typename T, int MEMORY_BUDGETS>
inline void Swap(Queue<T, MEMORY_BUDGETS>& a, Queue<T, MEMORY_BUDGETS>& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
