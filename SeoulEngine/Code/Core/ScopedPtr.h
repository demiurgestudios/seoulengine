/**
 * \file ScopedPtr.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCOPED_PTR_H
#define SCOPED_PTR_H

#include "CheckedPtr.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * ScopedPtr has the following semantics.
 * - If non-null, it calls SEOUL_DELETE on the pointer it contains
 *   when ScopedPtr's destructor is called.
 * - It is not copyable and not assignable.
 *
 * ScopedPtr is useful for usage scenarios where an object exclusively
 * owns a heap allocated object. ScopedPtr cannot be used in Vector<>s
 * of heap allocated objects or other containers that will copy their contents.
 */
template <typename T>
class ScopedPtr SEOUL_SEALED
{
public:
	ScopedPtr()
		: m_pObject(nullptr)
	{}

	ScopedPtr(ScopedPtr&& rp)
		: m_pObject(nullptr)
	{
		Swap(rp);
	}

	explicit ScopedPtr(T* p)
		: m_pObject(p)
	{}

	~ScopedPtr()
	{
		SafeDelete(m_pObject);
	}

	/**
	 * Assigns a new raw pointer to this ScopedPtr. If the ScopedPtr
	 * already contains a non-null pointer, SEOUL_DELETE will be called
	 * on that pointer.
	 */
	void Reset(T* pObject = nullptr)
	{
		T* p = m_pObject;
		m_pObject = nullptr;

		SafeDelete(p);
		m_pObject = pObject;
	}

	/**
	 * Accessor for the raw pointer stored in this ScopedPtr.
	 */
	T* Get() const
	{
		return m_pObject;
	}

	/**
	 * Returns true if this ScopedPtr's pointer is non-null, false
	 * otherwise.
	 */
	Bool IsValid() const
	{
		return (m_pObject != nullptr);
	}

	/**
	 * Dereferences the pointer stored in this ScopedPtr.
	 * \pre The pointer must be non-null.
	 */
	T& operator*() const
	{
		SEOUL_ASSERT(m_pObject != nullptr);
		return (*m_pObject);
	}

	/**
	 * Returns the pointer stored in this ScopedPtr for use by
	 * a -> operator.
	 *
	 * \pre The pointer must be non-null.
	 */
	T* operator->() const
	{
		SEOUL_ASSERT(m_pObject != nullptr);
		return (m_pObject);
	}

	/**
	 * Cheap swap between this ScopedPtr and a ScopedPtr b.
	 */
	void Swap(ScopedPtr& b)
	{
		T* pTemp = m_pObject;
		m_pObject = b.m_pObject;
		b.m_pObject = pTemp;
	}

	/**
	 * Swap between this ScopedPtr and a pointer of type T.
	 *
	 * \warning This is willfully abadoning ownership of the data
	 * by ScopedPtr. The caller takes ownership of the pointer and
	 * must destroy it with a call to SafeDelete().
	 */
	void Swap(CheckedPtr<T>& rp)
	{
		CheckedPtr<T> pTemp = m_pObject;
		m_pObject = rp;
		rp = pTemp;
	}

	/**
	 * Swap between this ScopedPtr and a pointer of type T.
	 *
	 * \warning This is willfully abadoning ownership of the data
	 * by ScopedPtr. The caller takes ownership of the pointer and
	 * must destroy it with a call to SafeDelete().
	 */
	void Swap(T*& rp)
	{
		T* pTemp = m_pObject;
		m_pObject = rp;
		rp = pTemp;
	}

private:
	T* m_pObject;

	SEOUL_DISABLE_COPY(ScopedPtr);
};

template <typename T, typename U>
inline Bool operator==(const ScopedPtr<T>& a, const ScopedPtr<U>& b)
{
	return (a.Get() == b.Get());
}

template <typename T, typename U>
inline Bool operator==(const ScopedPtr<T>& a, U* b)
{
	return (a.Get() == b);
}

template <typename T, typename U>
inline Bool operator==(U* a, const ScopedPtr<T>& b)
{
	return (a == b.Get());
}

template <typename T, typename U>
inline Bool operator!=(const ScopedPtr<T>& a, const ScopedPtr<U>& b)
{
	return (a.Get() != b.Get());
}

} // namespace Seoul

#endif // include guard
