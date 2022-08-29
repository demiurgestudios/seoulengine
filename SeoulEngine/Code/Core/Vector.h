/**
 * \file Vector.h
 * \brief SeoulEngine equivalent to std::vector<>. Identical to std::vector<> API, except:
 * - CamelCase function and members names.
 * - additional Append(const Vector).
 * - additional Fill(ConstReference val) method, equivalent to Assign(GetSize(), val).
 * - additional Contains(), and ContainsFromBack(), Find(), FindFromBack().
 * - additional RemoveFirstInstance().
 * - additional Get(SizeType) method, returns pointer.
 * - additional GetCapacityInBytes() and GetSizeInBytes().
 * - assign(size_type, const_reference val) includes a default argument for val.
 * - capacity() is called GetCapacity().
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
#ifndef	VECTOR_H
#define VECTOR_H

#include "Algorithms.h"
#include "Allocator.h"
#include "CheckedPtr.h"
#include "VectorInternal.h"
#include <initializer_list>

namespace Seoul
{

template <typename T, int MEMORY_BUDGETS>
class Vector
{
public:
	typedef VectorDetail::Buffer<T, MEMORY_BUDGETS>    BufferType;

	typedef typename BufferType::ValueType             ValueType;
	typedef typename BufferType::SizeType              SizeType;
	typedef typename BufferType::DifferenceType        DifferenceType;
	typedef typename BufferType::Pointer               Pointer;
	typedef typename BufferType::ConstPointer          ConstPointer;
	typedef typename BufferType::Reference             Reference;
	typedef typename BufferType::ConstReference        ConstReference;
	typedef typename BufferType::Iterator              iterator; /* range-based for loop */
	typedef typename BufferType::Iterator              Iterator;
	typedef typename BufferType::ConstIterator         const_iterator; /* range-based for loop */
	typedef typename BufferType::ConstIterator         ConstIterator;

	               Vector()                                             : m_Buffer() {}
	               Vector(const Vector& b)                              : m_Buffer(b.Begin(), b.End()) {}
	               template <int B_MEMORY_BUDGETS>
	               Vector(const Vector<T, B_MEMORY_BUDGETS>& b)         : m_Buffer(b.Begin(), b.End()) {}
	               template <typename ITER>
	               Vector(ITER begin, ITER end)                         : m_Buffer(begin, end) {}
	               Vector(const std::initializer_list<T>& b)            : m_Buffer(b.begin(), b.end()) {}
	explicit       Vector(SizeType n, ConstReference val = ValueType()) : m_Buffer(n, val) {}

	void           Append(const Vector& b)                              { Insert(End(), b.Begin(), b.End()); }
	               template <int B_MEMORY_BUDGETS>
	void           Append(const Vector<T, B_MEMORY_BUDGETS>& b)         { Insert(End(), b.Begin(), b.End()); }
	               template <typename ITER>
	void           Append(ITER begin, ITER end)                         { Insert(End(), begin, end); }

	void           Assign(SizeType n, ConstReference val = ValueType()) { m_Buffer.Assign(n, val); }
	               template <typename ITER>
	void           Assign(ITER begin, ITER end)                         { m_Buffer.Assign(begin, end); }

	Reference      At(SizeType n)                                       { return m_Buffer.At(n); }
	ConstReference At(SizeType n) const                                 { return m_Buffer.At(n); }

	Reference      Back()                                               { return m_Buffer.Back(); }
	ConstReference Back() const                                         { return m_Buffer.Back(); }

	iterator       begin() /* range-based for loop */                   { return Begin(); }
	const_iterator begin() const /* range-based for loop */             { return Begin(); }
	Iterator       Begin()                                              { return m_Buffer.Begin(); }
	ConstIterator  Begin() const                                        { return m_Buffer.Begin(); }

	void           Clear()                                              { m_Buffer.Clear(); }

	Bool           Contains(ConstReference val) const                   { return Seoul::Contains(Begin(), End(), val); }
	               template <typename U>
	Bool           Contains(const U& val) const                         { return Seoul::Contains(Begin(), End(), val); }

	Bool           ContainsFromBack(ConstReference val) const           { return Seoul::ContainsFromBack(Begin(), End(), val); }
	               template <typename U>
	Bool           ContainsFromBack(const U& val) const                 { return Seoul::ContainsFromBack(Begin(), End(), val); }

	Pointer        Data()                                               { return m_Buffer.Data(); }
	ConstPointer   Data() const                                         { return m_Buffer.Data(); }

	iterator       end() /* range-based for loop */                     { return End(); }
	const_iterator end() const /* range-based for loop */               { return End(); }
	Iterator       End()                                                { return m_Buffer.End(); }
	ConstIterator  End() const                                          { return m_Buffer.End(); }

	Iterator       Erase(Iterator pos)                                  { return m_Buffer.Erase(pos, pos + 1); }
	Iterator       Erase(Iterator begin, Iterator end)                  { return m_Buffer.Erase(begin, end); }

	void           Fill(ConstReference val)                             { Assign(GetSize(), val); }

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

	Reference      Front()                                              { return m_Buffer.Front(); }
	ConstReference Front() const                                        { return m_Buffer.Front(); }

	Pointer        Get(SizeType n)                                      { return m_Buffer.Get(n); }
	ConstPointer   Get(SizeType n) const                                { return m_Buffer.Get(n); }

	SizeType       GetCapacity() const                                  { return m_Buffer.GetCapacity(); }
	SizeType       GetCapacityInBytes() const                           { return m_Buffer.GetCapacityInBytes(); }
	SizeType       GetSize() const                                      { return m_Buffer.GetSize(); }
	SizeType       GetSizeInBytes() const                               { return m_Buffer.GetSizeInBytes(); }

	Iterator       Insert(Iterator pos, ConstReference val)                     { return m_Buffer.Insert(pos, val); }
	Iterator       Insert(Iterator pos, ValueType&& rval)                       { return m_Buffer.Insert(pos, RvalRef(rval)); }
	void           Insert(Iterator pos, SizeType n, ConstReference val)         { m_Buffer.Insert(pos, n, val); }
	void           Insert(Iterator pos, ConstIterator begin, ConstIterator end) { m_Buffer.Insert(pos, begin, end); }

	Bool           IsEmpty() const { return m_Buffer.IsEmpty(); }

	Vector&        operator=(const Vector& b)                      { Assign(b.Begin(), b.End()); return *this; }
	               template <int B_MEMORY_BUDGETS>
	Vector&        operator=(const Vector<T, B_MEMORY_BUDGETS>& b) { Assign(b.Begin(), b.End()); return *this; }

	Reference      operator[](SizeType n)                          { return m_Buffer[n]; }
	ConstReference operator[](SizeType n) const                    { return m_Buffer[n]; }

	void           PopBack()                                       { SEOUL_ASSERT(!IsEmpty()); Erase(End() - 1); }
	void           PushBack(ConstReference val)                    { m_Buffer.Insert(End(), &val, &val + 1); }
	void           PushBack(ValueType&& rval)                      { m_Buffer.Insert(End(), RvalRef(rval)); }

	UInt32         Remove(ConstReference val);
	               template <typename U>
	UInt32         Remove(const U& val);
	               template <typename PRED>
	UInt32         RemoveIf(PRED pred);
	Bool           RemoveFirstInstance(ConstReference val);
	               template <typename U>
	Bool           RemoveFirstInstance(const U& val);

	void           Reserve(SizeType n)                             { m_Buffer.Reserve(n); }

	void           Resize(SizeType n)                              { m_Buffer.Resize(n); }
	void           Resize(SizeType n, ConstReference val)          { m_Buffer.Resize(n, val); }

	void           ShrinkToFit()                                   { m_Buffer.ShrinkToFit(); }

	void           Swap(Vector& b)                                 { m_Buffer.Swap(b.m_Buffer); }

private:
	BufferType m_Buffer;
}; // class Vector

template <typename T, int MEMORY_BUDGETS>
inline UInt32 Vector<T, MEMORY_BUDGETS>::Remove(ConstReference val)
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
inline UInt32 Vector<T, MEMORY_BUDGETS>::Remove(const U& val)
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
inline UInt32 Vector<T, MEMORY_BUDGETS>::RemoveIf(PRED pred)
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
inline Bool Vector<T, MEMORY_BUDGETS>::RemoveFirstInstance(ConstReference val)
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
inline Bool Vector<T, MEMORY_BUDGETS>::RemoveFirstInstance(const U& val)
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
inline Bool operator==(const Vector<TA, MEMORY_BUDGETS_A>& vA, const Vector<TB, MEMORY_BUDGETS_B>& vB)
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
inline Bool operator!=(const Vector<TA, MEMORY_BUDGETS_A>& vA, const Vector<TB, MEMORY_BUDGETS_B>& vB)
{
	return !(vA == vB);
}

/**
 * Deletes heap allocated objects referenced in the vector rVector,
 * and then clears the vector.
 */
template <typename T, int MEMORY_BUDGETS>
inline void SafeDeleteVector(Vector<T*, MEMORY_BUDGETS>& rvVector)
{
	for (Int i = (Int)rvVector.GetSize() - 1; i >= 0; --i)
	{
		SafeDelete(rvVector[i]);
	}
	rvVector.Clear();
}

/**
 * Deletes heap allocated objects referenced in the vector rVector,
 * and then clears the vector.
 */
template <typename T, int MEMORY_BUDGETS>
inline void SafeDeleteVector(Vector< CheckedPtr<T>, MEMORY_BUDGETS>& rvVector)
{
	for (Int i = (Int)rvVector.GetSize() - 1; i >= 0; --i)
	{
		SafeDelete(rvVector[i]);
	}
	rvVector.Clear();
}

/**
 * Equivalent to std::swap(). Override specifically for Vector<>.
 */
template <typename T, int MEMORY_BUDGETS>
inline void Swap(Vector<T, MEMORY_BUDGETS>& a, Vector<T, MEMORY_BUDGETS>& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
