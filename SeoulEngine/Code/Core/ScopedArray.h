/**
 * \file ScopedArray.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCOPED_ARRAY_H
#define SCOPED_ARRAY_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * ScopedArray has the following semantics.
 * - If non-null, it calls SEOUL_DELETE [] on the pointer it contains
 *   when ScopedArray's destructor is called.
 * - It is not copyable and not assignable.
 *
 * ScopedArray is useful for usage scenarios where an object exclusively
 * owns a heap allocated object. ScopedArray cannot be used in Vector<>s
 * of heap allocated objects or other containers that will copy their contents.
 */
template <typename T>
class ScopedArray SEOUL_SEALED
{
public:
	ScopedArray()
		: m_pObject(nullptr)
	{}

	explicit ScopedArray(T* p)
		: m_pObject(p)
	{}

	~ScopedArray()
	{
		SafeDeleteArray(m_pObject);
	}

	/**
	 * Assigns a new raw pointer to this ScopedArray. If the ScopedArray
	 * already contains a non-null pointer, SEOUL_DELETE will be called
	 * on that pointer.
	 */
	void Reset(T* pObject = nullptr)
	{
		T* p = m_pObject;
		m_pObject = nullptr;

		SafeDeleteArray(p);
		m_pObject = pObject;
	}

	/**
	 * Accessor for the raw pointer stored in this ScopedArray.
	 */
	T* Get() const
	{
		return m_pObject;
	}

	/**
	 * Array accessor, read-write.
	 */
	T& operator[](Int32 i)
	{
		return *(m_pObject + i);
	}

	/**
	 * Array accessor, read-only.
	 */
	const T& operator[](Int32 i) const
	{
		return *(m_pObject + i);
	}

	/**
	 * Returns true if this ScopedArray's pointer is non-null, false
	 * otherwise.
	 */
	Bool IsValid() const
	{
		return (m_pObject != nullptr);
	}

	/**
	 * Cheap swap between this ScopedArray and a ScopedArray b.
	 */
	void Swap(ScopedArray& b)
	{
		T* pTemp = m_pObject;
		m_pObject = b.m_pObject;
		b.m_pObject = pTemp;
	}

private:
	T* m_pObject;

	SEOUL_DISABLE_COPY(ScopedArray);
};

template <typename T, typename U>
inline Bool operator==(const ScopedArray<T>& a, const ScopedArray<U>& b)
{
	return (a.Get() == b.Get());
}

template <typename T, typename U>
inline Bool operator==(const ScopedArray<T>& a, U* b)
{
	return (a.Get() == b);
}

template <typename T, typename U>
inline Bool operator==(U* a, const ScopedArray<T>& b)
{
	return (a == b.Get());
}

template <typename T, typename U>
inline Bool operator!=(const ScopedArray<T>& a, const ScopedArray<U>& b)
{
	return (a.Get() != b.Get());
}

} // namespace Seoul

#endif // include guard
