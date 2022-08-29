/**
 * \file SharedPtr.h
 * \brief "Smart" ptr that handles thread-safe reference counting of
 * a heap allocated object. Objects will be automatically destroyed
 * with a call to SEOUL_DELETE when their ref count == 0.
 *
 * The "thread-safe" quality of SharedPtr is limited to the reference
 * count, and this is, strictly speaking, provided not by SharedPtr but
 * by the Atomic32 class, which is used for reference counting
 * in the default implementation of GlobalIncrementReferenceCount,
 * GlobalDecrementReferenceCount, and GlobalGetReferenceCount. As a result,
 * if you provide custom implementations of these methods, you must
 * use Atomic32 or an equivalent implementation to maintain the
 * thread safety of SharedPtr.
 *
 * \usage SharedPtr<> is useful in contexts where an object's lifespan
 * is dependent on another object. For cases where lifespan is separate
 * but dangling references are a problem, try a Handle<> instead.
 *
 * \usage Objects should be pass by regular pointer ("weak" pointer) in
 * function arguments. In any case where the pointer is stored (local
 * to a function, members of a class), use a SharedPtr<>.
 *
 * \usage Although SharedPtr<>'s are thread-safe, in that two SharedPtr<>s
 * to the same object can exist on separate threads and the reference
 * count will be updated correctly, they provide no other thread safety. Other
 * mechanisms must be employed to make sure object mutations are thread-safe
 * and accessors are thread-safe.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include "AtomicPointer.h"
#include "Atomic32.h"
#include "Prereqs.h"

namespace Seoul
{

template <typename T> void SeoulGlobalIncrementReferenceCount(const T* p);
template <typename T> Int32 SeoulGlobalGetReferenceCount(const T* p);
template <typename T> void SeoulGlobalDecrementReferenceCount(const T* p);

/**
 * Managed ref counting of heap allocated objects.
 *
 * \usage To use SharedPtr, a class must friend GlobalIncrementReferenceCount,
 * GlobalDecrementReferenceCount, and GlobalGetReferenceCount. These functions can be
 * overriden specially for a particular class or the default
 * templates can be used. SEOUL_REFERENCE_COUNTED() is provided for this purpose.
 *
 * \warning If a specialized version of GlobalIncrementReferenceCount, GlobalDecrementReferenceCount,
 * or GlobalGetReferenceCount is used, the implementations must be thread safe. This can be
 * accomplished by using Atomic32.
 */

/**
 * SharedPtrImpl has the meat of the SharedPtr implementation.
 * This shouldn't be used by anything directly except for SharedPtr itself.
 * 
 * DerivedMngdPtrType is the fully qualified SharedPtr type (eg. SharedPtr<HashSet<HString> >) where
 * ValueType is the managed type (eg. HashSet<HString> in the previous example).
 */
template <typename  DerivedMngdPtrType, typename ValueType>
class SharedPtrImpl SEOUL_ABSTRACT
{
public:
	SharedPtrImpl()
		: m_p(nullptr)
	{
	}

	/**
	 * Initialize this SharedPtr with a raw pointer of type T.
	 * If bIncrementReferenceCount() is true, the reference count of a valid p
	 * will be incremented, otherwise it will remain unchanged.
	 */
	explicit SharedPtrImpl(ValueType* p)
		: m_p(p)
	{
		if (m_p)
		{
			::Seoul::SeoulGlobalIncrementReferenceCount(m_p);
		}
	}

	/**
	 * Instantiate this SharedPtr with a reference
	 * to the object of b, incrementing the reference count
	 * by 1 if b is a valid pointer.
	 */
	SharedPtrImpl(const SharedPtrImpl& b)
		: m_p(b.m_p)
	{
		if (m_p)
		{
			::Seoul::SeoulGlobalIncrementReferenceCount(m_p);
		}
	}

	template <typename U, typename V>
	SharedPtrImpl(const SharedPtrImpl<U, V>& b)
		: m_p(b.GetPtr())
	{
		if (m_p)
		{
			::Seoul::SeoulGlobalIncrementReferenceCount(m_p);
		}
	}

	~SharedPtrImpl()
	{
		if (m_p)
		{
			::Seoul::SeoulGlobalDecrementReferenceCount(m_p);
			m_p = nullptr;
		}
	}

	SharedPtrImpl& operator=(const SharedPtrImpl& b)
	{
		Reset(b.GetPtr());

		return *this;
	}

	template <typename U, typename V>
	SharedPtrImpl& operator=(const SharedPtrImpl<U, V>& b)
	{
		Reset(b.GetPtr());

		return *this;
	}

	/**
	 * Sets the pointer of this SharedPtr(),
	 * incrementing and decrementing associated pointers
	 * appropriately.
	 */
	void Reset(ValueType* pObject = nullptr)
	{
		if (pObject == m_p)
		{
			return;
		}

		ValueType* p = m_p;
		m_p = nullptr;

		// Decrement the existing pointer.
		if (p)
		{
			::Seoul::SeoulGlobalDecrementReferenceCount(p);
		}

		p = pObject;

		// Increment the new pointer.
		if (p)
		{
			::Seoul::SeoulGlobalIncrementReferenceCount(p);
		}

		// Assign the new pointer.
		m_p = p;
	}

	/**
	 * Updates the pointer of this SharedPtr()
	 * to pObject as an atomic operation - the replaced
	 * pointer is returned.
	 */
	DerivedMngdPtrType AtomicReplace(ValueType* pObject = nullptr)
	{
		if (pObject)
		{
			::Seoul::SeoulGlobalIncrementReferenceCount(pObject);
		}

		ValueType* p = m_p;
		while (p != AtomicPointerCommon::AtomicSetIfEqual(
			(void**)&m_p,
			(void*)pObject,
			(void*)p))
		{
			p = m_p;
		}

		// Don't use the constructor here, since we did not decrement
		// the reference count.
		DerivedMngdPtrType pReturn;
		pReturn.m_p = p;
		return pReturn;
	}

	/**
	 * Return this SharedPtr<>'s pointer.
	 */
	ValueType* GetPtr() const
	{
		return m_p;
	}

	/**
	 * @return The reference count of the object pointed at
	 * by this SharedPtr<>, or 0 if this SharedPtr<>
	 * contains a nullptr pointer.
	 */
	Int32 GetReferenceCount() const
	{
		return (m_p)
			? ::Seoul::SeoulGlobalGetReferenceCount(m_p)
			: 0;
	}

	/**
	 * @return True if the reference count of this SharedPtr<> is
	 * 1, or false if it is not, or this SharedPtr<> contains the
	 * nullptr pointer.
	 */
	Bool IsUnique() const
	{
		return (m_p)
			? (1 == ::Seoul::SeoulGlobalGetReferenceCount(m_p))
			: false;
	}

	/**
	 * @return If this SharedPtr<> points at a valid object.
	 */
	Bool IsValid() const
	{
		return (nullptr != m_p);
	}

	/**
	 * Cheap swap between this SharedPtr and a SharedPtr b.
	 */
	void Swap(SharedPtrImpl& b)
	{
		ValueType* pTemp = m_p;
		m_p = b.m_p;
		b.m_p = pTemp;
	}

	/**
	 * Cheap swap between this SharedPtr and a SharedPtr<U> b.
	 */
	template <typename U, typename V>
	void Swap(SharedPtrImpl<U, V>& b)
	{
		ValueType* pTemp = m_p;
		m_p = b.m_p;
		b.m_p = pTemp;
	}

private:
	template <typename U, typename V>
	friend class ManagedPtrImpl;

	ValueType* m_p;
}; // class SharedPtrImpl


template <typename T>
class SharedPtr SEOUL_SEALED : public SharedPtrImpl<SharedPtr<T>, T>
{
	typedef SharedPtrImpl<SharedPtr<T>, T> BaseImpl;

public:
	SharedPtr() : BaseImpl()
	{
	}

	/**
	 * Initialize this SharedPtr with a raw pointer of type T.
	 * If bIncrementReferenceCount() is true, the reference count of a valid p
	 * will be incremented, otherwise it will remain unchanged.
	 */
    explicit SharedPtr(T* p) : BaseImpl(p)
    {
	}

	/**
	 * Instantiate this SharedPtr with a reference
	 * to the object of b, incrementing the reference count
	 * by 1 if b is a valid pointer.
	 */
    SharedPtr(const SharedPtr& b) : BaseImpl(b.GetPtr())
    {
    }

    template <typename U>
    SharedPtr(const SharedPtr<U>& b) : BaseImpl(b.GetPtr())
    {
    }

	T& operator*() const
    {
		SEOUL_ASSERT(nullptr != BaseImpl::GetPtr());

		return *BaseImpl::GetPtr();
    }

    T* operator->() const
    {
		SEOUL_ASSERT(nullptr != BaseImpl::GetPtr());

		return BaseImpl::GetPtr();
	}

private:
	template <typename U>
	friend class SharedPtr;
};

template <typename T, typename U>
inline Bool operator==(const SharedPtr<T>& a, const SharedPtr<U>& b)
{
	return (a.GetPtr() == b.GetPtr());
}

template <typename T, typename U>
inline Bool operator==(const SharedPtr<T>& a, U* b)
{
	return (a.GetPtr() == b);
}

template <typename T, typename U>
inline Bool operator==(U* a, const SharedPtr<T>& b)
{
	return (a == b.GetPtr());
}

template <typename T, typename U>
inline Bool operator!=(const SharedPtr<T>& a, const SharedPtr<U>& b)
{
	return (a.GetPtr() != b.GetPtr());
}

/**
 * Convenience specialization of the global Swap<> function, used
 * to call SharedPtr<T> member function.
 */
template <typename T>
inline void Swap(SharedPtr<T>& a, SharedPtr<T>& b)
{
	a.Swap(b);
}

/**
 * Default implementation of the hook used to increment an object's
 * internal reference count.
 */
template <typename T>
void SeoulGlobalIncrementReferenceCount(const T* pIn)
{
	T* p = const_cast<T*>(pIn);
	SEOUL_ASSERT(nullptr != p);
	++(p->m_AtomicReferenceCount);
}

/**
 * Default implementation of the hook used to acquire an object's
 * internal reference count.
 */
template <typename T>
Int32 SeoulGlobalGetReferenceCount(const T* p)
{
	SEOUL_ASSERT(nullptr != p);
	return (Int32)(p->m_AtomicReferenceCount);
}

/**
 * Default implementation of the hook used to decrement an object's
 * internal reference count. May destroy the object with a call
 * to SEOUL_DELETE if the object's reference count reaches 0.
 */
template <typename T>
void SeoulGlobalDecrementReferenceCount(const T* pIn)
{
	T* p = const_cast<T*>(pIn);
	SEOUL_ASSERT(nullptr != p);
	SEOUL_ASSERT(SeoulGlobalGetReferenceCount(pIn) > 0);

	// IMPORTANT: To respect atomicity, we can only make decisions
	// on the value returned from the operation - in doing so,
	// only the thread that causes the count to reach 0 will
	// have a value of 0 set to count, and the deletion of p
	// will happen only once.
	Atomic32Type const count = --(p->m_AtomicReferenceCount);

	if (0 == count)
	{
		SEOUL_DELETE p;
		p = nullptr;
	}
}

/**
 * Macro to simplify defining a class as reference counted. Use
 * this macro in the base class that contains the counter variable -
 * should be placed in the private section of sealed classes or
 * in the protected section of polymorphic classes.
 */
#define SEOUL_REFERENCE_COUNTED(class_name) \
	friend void Seoul::SeoulGlobalIncrementReferenceCount<class_name>(const class_name* p); \
	friend Int32 Seoul::SeoulGlobalGetReferenceCount<class_name>(const class_name* p); \
	friend void Seoul::SeoulGlobalDecrementReferenceCount<class_name>(const class_name* p); \
	\
	Atomic32 m_AtomicReferenceCount

/**
 * Macro to simplify defining a subclass of a reference counted
 * base class. Should be placed in the protected section of the class.
 */
#define SEOUL_REFERENCE_COUNTED_SUBCLASS(class_name) \
	friend void Seoul::SeoulGlobalIncrementReferenceCount<class_name>(const class_name* p); \
	friend Int32 Seoul::SeoulGlobalGetReferenceCount<class_name>(const class_name* p); \
	friend void Seoul::SeoulGlobalDecrementReferenceCount<class_name>(const class_name* p)

} // namespace Seoul

#endif // include guard
