/**
 * \file ChekcedPtr.h
 *
 * \brief CheckedPtr is a weak wrapper around a regular C++ pointer - unlike
 * ScopedPtr, it does not managed the memory it points to. However, also
 * unlike ScopedPtr, it can more implicitly replace a regular pointer. The purpose
 * of CheckedPtr is to lightly replace regular pointers, while providing:
 * - default construction to nullptr
 * - SEOUL_ASSERT() checking of operator-> and operator*
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CHECKED_PTR_H
#define CHECKED_PTR_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * CheckedPtr has the following semantics:
 * - default construct to nullptr
 * - SEOUL_ASSERT() check of operator-> and operator*
 * - otherwise behaves like a regular C++ pointer - it provides no automatic
 *   management of heap memory. For that, \sa ScopedPtr.
 */
template <typename T>
class CheckedPtr SEOUL_SEALED
{
public:
	CheckedPtr()
		: m_pObject(nullptr)
	{
	}

	CheckedPtr(T* p)
		: m_pObject(p)
	{
	}

	template <typename U>
	CheckedPtr(const CheckedPtr<U>& b)
		: m_pObject(b.Get())
	{
	}

	template <typename U>
	CheckedPtr& operator=(U* p)
	{
		m_pObject = p;
		return *this;
	}

	template <typename U>
	CheckedPtr& operator=(const CheckedPtr<U>& b)
	{
		m_pObject = b.Get();
		return *this;
	}

	~CheckedPtr()
	{
	}

	/**
	 * @return this CheckedPtr cast to its underlying pointer type T.
	 */
	operator T*() const
	{
		return Get();
	}

	/**
	 * Assigns a new raw pointer to this CheckedPtr of its underlying type T.
	 */
	void Reset(T* pObject = nullptr)
	{
		m_pObject = pObject;
	}

	/**
	 * @return The raw pointer of this CheckedPtr.
	 */
	T* Get() const
	{
		return m_pObject;
	}

	/**
	 * @return True if this CheckedPtr is non-null, false otherwise.
	 */
	Bool IsValid() const
	{
		return (m_pObject != nullptr);
	}

	/**
	 * Dereferences the pointer stored in this CheckedPtr.
	 * \pre The pointer must be non-null.
	 */
	T& operator*() const
	{
		SEOUL_ASSERT(m_pObject != nullptr);
		return (*m_pObject);
	}

	/**
	 * Returns the pointer stored in this CheckedPtr for use by
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
	 * Exchange the value stored in this CheckedPtr with
	 * the value stored in b.
	 */
	void Swap(CheckedPtr& b)
	{
		T* pTemp = m_pObject;
		m_pObject = b.m_pObject;
		b.m_pObject = pTemp;
	}

	/**
	 * Exchange the value stored in this CheckedPtr with
	 * the value stored in rp.
	 */
	void Swap(T*& rp)
	{
		T* pTemp = m_pObject;
		m_pObject = rp;
		rp = pTemp;
	}

private:
	T* m_pObject;
}; // class CheckedPtr.

template <typename T, typename U>
inline Bool operator==(const CheckedPtr<T>& a, const CheckedPtr<U>& b)
{
	return (a.Get() == b.Get());
}

template <typename T, typename U>
inline Bool operator==(const CheckedPtr<T>& a, U* b)
{
	return (a.Get() == b);
}

template <typename T, typename U>
inline Bool operator==(U* a, const CheckedPtr<T>& b)
{
	return (a == b.Get());
}

template <typename T, typename U>
inline Bool operator!=(const CheckedPtr<T>& a, const CheckedPtr<U>& b)
{
	return (a.Get() != b.Get());
}

template <typename T, typename U>
inline Bool operator!=(const CheckedPtr<T>& a, U* b)
{
	return (a.Get() != b);
}

template <typename T, typename U>
inline Bool operator!=(U* a, const CheckedPtr<T>& b)
{
	return (a != b.Get());
}

/**
 * Deletes a heap allocated object and sets the pointer to nullptr
 */
template <typename T>
inline void SafeDelete(CheckedPtr<T>& rpPointer)
{
	T* p = rpPointer.Get();
	rpPointer.Reset();

	SEOUL_DELETE p;
}


/**
 * Calls AddRef() on a valid pointer and returns the reference
 * count returned by the AddRef() function.
 *
 * Returns 0u and does nothing if pPointer is nullptr.
 */
template <typename T>
UInt SafeAcquire(CheckedPtr<T> pPointer)
{
	if (pPointer)
	{
		return pPointer->AddRef();
	}

	return 0u;
}

/**
 * Calls release on a valid pointer pPointer (does nothing if the
 * pointer is nullptr), and sets ap to nullptr.
 */
template <typename T>
UInt SafeRelease(CheckedPtr<T>& rpPointer)
{
	CheckedPtr<T> p = rpPointer;
	rpPointer = nullptr;

	if (p)
	{
		return p->Release();
	}

	return 0u;
}

/**
 * Because CheckedPtr has no special copy operations and always contains
 * a plain pointer, it can be treated as simple.
 */
template <typename T> struct CanMemCpy< CheckedPtr<T> > { static const Bool Value = true; };
template <typename T> struct CanZeroInit< CheckedPtr<T> > { static const Bool Value = true; };

} // namespace Seoul

#endif // include guard
