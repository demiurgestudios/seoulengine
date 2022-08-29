/**
 * \file BoxedValue.h
 * \brief Wrap an object so it can be used and tracked by
 * SharedPtr<>.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BOXED_VALUE_H
#define BOXED_VALUE_H

#include "SharedPtr.h"

namespace Seoul
{

template <class T>
class BoxedValue SEOUL_SEALED
{
public:
	// Default constructor
	BoxedValue()
	{
	}

	explicit BoxedValue(T const& value)
		: m_Value(value)
	{
	}

	T& GetBoxedValue()
	{
		return m_Value;
	}

	const T& GetBoxedValue() const
	{
		return m_Value;
	}

private:
	// Boxed value
	T m_Value;

	// Allow this object to be used reference counted.
	SEOUL_REFERENCE_COUNTED(BoxedValue<T>);

	SEOUL_DISABLE_COPY(BoxedValue);
}; // class BoxedValue

/**
 * Specialized versions of SharedPtr<> when pointing to a BoxedValue.
 * Since the BoxedValue exists just to wrap the internal type,
 * this specialized version will act on the internal value rather than
 * the BoxedValue itself when the SharedPtr<> is dereferenced.
 *
 * This allows a SharedPtr<> of type SharedPtr< BoxedValue<T> > or
 * SharedPtr<const BoxedValue<T> > to be used as if it was a SharedPtr<T>.
 */
template <typename T>
class SharedPtr< BoxedValue<T> > SEOUL_SEALED : public SharedPtrImpl< SharedPtr< BoxedValue<T> >, BoxedValue<T> >
{
	typedef SharedPtrImpl<SharedPtr< BoxedValue<T> >, BoxedValue<T> > BaseImpl;

public:
	SharedPtr()
		: BaseImpl()
	{
	}

	/**
	 * Initialize this SharedPtr with a raw pointer of type T.
	 * If bIncrementReferenceCount() is true, the reference count of a valid p
	 * will be incremented, otherwise it will remain unchanged.
	 */
	explicit SharedPtr(BoxedValue<T>* p)
		: BaseImpl(p)
	{
	}

	/**
	 * Instantiate this SharedPtr with a reference
	 * to the object of b, incrementing the reference count
	 * by 1 if b is a valid pointer.
	 */
	SharedPtr(const SharedPtr& b)
		: BaseImpl(b.GetPtr())
	{
	}

	template <typename U>
	SharedPtr(const SharedPtr<U>& b)
		: BaseImpl(b.GetPtr())
	{
	}

	T& operator*() const
	{
		SEOUL_ASSERT(nullptr != BaseImpl::GetPtr());

		return BaseImpl::GetPtr()->GetBoxedValue();
	}

	T* operator->() const
	{
		SEOUL_ASSERT(nullptr != BaseImpl::GetPtr());

		return &BaseImpl::GetPtr()->GetBoxedValue();
	}

private:
	template <typename U>
	friend class SharedPtr;
};

template <typename T>
class SharedPtr< const BoxedValue<T> > SEOUL_SEALED : public SharedPtrImpl< SharedPtr< const BoxedValue<T> >, const BoxedValue<T> >
{
	typedef SharedPtrImpl< SharedPtr<const BoxedValue<T> >, const BoxedValue<T> > BaseImpl;

public:
	SharedPtr()
		: BaseImpl()
	{
	}

	/**
	 * Initialize this SharedPtr with a raw pointer of type T.
	 * If bIncrementReferenceCount() is true, the reference count of a valid p
	 * will be incremented, otherwise it will remain unchanged.
	 */
	explicit SharedPtr(const BoxedValue<T>* p)
		: BaseImpl(p)
	{
	}

	/**
	 * Instantiate this SharedPtr with a reference
	 * to the object of b, incrementing the reference count
	 * by 1 if b is a valid pointer.
	 */
	SharedPtr(const SharedPtr& b)
		: BaseImpl(b.GetPtr())
	{
	}

	template <typename U>
	SharedPtr(const SharedPtr<U>& b)
		: BaseImpl(b.GetPtr())
	{
	}

	const T& operator*() const
	{
		SEOUL_ASSERT(nullptr != BaseImpl::GetPtr());

		return BaseImpl::GetPtr()->GetBoxedValue();
	}

	const T* operator->() const
	{
		SEOUL_ASSERT(nullptr != BaseImpl::GetPtr());

		return &BaseImpl::GetPtr()->GetBoxedValue();
	}

private:
	template <typename U>
	friend class SharedPtr;
};

} // namespace Seoul

#endif // include guard
