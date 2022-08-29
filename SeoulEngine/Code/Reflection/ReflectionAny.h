/**
 * \file ReflectionAny.h
 * \brief Any is a concrete class that can wrap any type. Unlike WeakAny,
 * Any always makes a copy of the source object. It is therefore safe
 * to use in any context, but often more computationally expensive
 * than WeakAny.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_ANY_H
#define REFLECTION_ANY_H

#include "DataStore.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "ReflectionTypeInfo.h"
#include "ReflectionWeakAny.h"
#include "ScopedPtr.h"

namespace Seoul::Reflection
{

/** Alignment of Any's internal storage. */
#define SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT 8

/** Size of Any's internal storage. */
#define SEOUL_ANY_INTERNAL_STORAGE_SIZE 32

/** Size of the largest type Any can store internally. */
#define SEOUL_ANY_LARGEST_TYPE_SIZE (SEOUL_ANY_INTERNAL_STORAGE_SIZE - SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT)

/** Macro to reduce clutter. */
#define SEOUL_ANY_IN_PLACE(type) (SEOUL_ALIGN_OF(type) <= SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT && sizeof(type) <= SEOUL_ANY_LARGEST_TYPE_SIZE)

namespace AnyDetail
{

/**
 * Wrapper which encloses the value stored within an Any object.
 */
struct BasePlaceholder SEOUL_ABSTRACT
{
	virtual ~BasePlaceholder()
	{
	}

	// Called when the value contained within this placeholder must
	// be cloned to the internal storage area of another placeholder.
	virtual void Clone(Byte aInternalValue[SEOUL_ANY_INTERNAL_STORAGE_SIZE]) const = 0;

	// Called when the value contained with this placeholder must
	// be cloned to a raw allocated buffer.
	virtual bool Clone(void* pData, size_t zDataSizeInBytes) const = 0;

	// Return a WeakAny to the object contain in this BasePlaceholder
	virtual WeakAny GetWeakAny() const = 0;

	// Return a WeakAny to a writeable pointer to the object contained in this BasePlaceholder
	virtual WeakAny GetWeakAnyPointerToValue() const = 0;

	// Return a WeakAny to a const pointer to the object contained in this BasePlaceholder
	virtual WeakAny GetWeakAnyConstPointerToValue() const = 0;

	// Return a TypeInfo object describing the object stored within
	// this placeholder.
	virtual const TypeInfo& GetTypeInfo() const = 0;

	// Return a WeakAny wrapper around a typed pointer to the object
	// contained within this placeholder.
	virtual WeakAny GetPointerToObject() const = 0;

	// Cast a pointer to the object contained within this placeholder
	// to a void*.
	virtual void const* GetConstVoidStarPointerToObject() const = 0;
}; // struct BasePlaceholder

// Forward declaration - if B_IN_PLACE_STORAGE is true,
// Placeholder will be specialized to copy the value into
// the Placeholder itself instead of using heap allocation. This
// is only valid for types for which SEOUL_ANY_IN_PLACE() is true.
template <typename T, Bool B_IN_PLACE_STORAGE>
struct Placeholder;

/**
 * Template variation of Placeholder than copies the
 * enclosed value in-place.
 */
template <typename T>
struct Placeholder<T, true> SEOUL_SEALED: public BasePlaceholder
{
	Placeholder(const T& value)
		: m_Value(value)
	{
	}

	virtual void Clone(Byte aInternalValue[SEOUL_ANY_INTERNAL_STORAGE_SIZE]) const SEOUL_OVERRIDE
	{
		// Instantiate a new in-place Placeholder with the value contained in this Placeholder.
		SEOUL_STATIC_ASSERT(sizeof(Placeholder<T, true>) <= SEOUL_ANY_INTERNAL_STORAGE_SIZE);
		new (reinterpret_cast<Placeholder<T, true>*>(aInternalValue)) Placeholder<T, true>(m_Value);
	}

	virtual bool Clone(void* pData, size_t zDataSizeInBytes) const SEOUL_OVERRIDE
	{
		if ((size_t)pData % SEOUL_ALIGN_OF(T) != 0 ||
			zDataSizeInBytes < sizeof(T))
		{
			return false;
		}

		new (pData) T(m_Value);
		return true;
	}

	virtual const TypeInfo& GetTypeInfo() const SEOUL_OVERRIDE
	{
		return TypeId<T>();
	}

	virtual WeakAny GetWeakAny() const SEOUL_OVERRIDE
	{
		return WeakAny(GetValue());
	}

	virtual WeakAny GetWeakAnyPointerToValue() const SEOUL_OVERRIDE
	{
		return WeakAny(&const_cast<T&>(GetValue()));
	}

	virtual WeakAny GetWeakAnyConstPointerToValue() const SEOUL_OVERRIDE
	{
		return WeakAny(&const_cast<T const&>(GetValue()));
	}

	T& GetValue() const
	{
		return const_cast<T&>(m_Value);
	}

	virtual WeakAny GetPointerToObject() const SEOUL_OVERRIDE
	{
		return (&const_cast<T&>(m_Value));
	}

	virtual void const* GetConstVoidStarPointerToObject() const SEOUL_OVERRIDE
	{
		return reinterpret_cast<void const*>(&m_Value);
	}

private:
	T m_Value;

	SEOUL_DISABLE_COPY(Placeholder);
}; // struct Placeholder

/**
 * Template variation of Placeholder than copies the
 * enclosed value to a heap memory area.
 */
template <typename T>
struct Placeholder<T, false> SEOUL_SEALED: public BasePlaceholder
{
	Placeholder(const T& value)
		: m_pValue(SEOUL_NEW(MemoryBudgets::Reflection) T(value))
	{
	}

	virtual void Clone(Byte aInternalValue[SEOUL_ANY_INTERNAL_STORAGE_SIZE]) const SEOUL_OVERRIDE
	{
		SEOUL_STATIC_ASSERT(sizeof(Placeholder<T, false>) <= SEOUL_ANY_INTERNAL_STORAGE_SIZE);
		new (reinterpret_cast<Placeholder<T, false>*>(aInternalValue)) Placeholder<T, false>(*m_pValue);
	}

	virtual bool Clone(void* pData, size_t zDataSizeInBytes) const SEOUL_OVERRIDE
	{
		if ((size_t)pData % SEOUL_ALIGN_OF(T) != 0 ||
			zDataSizeInBytes < sizeof(T))
		{
			return false;
		}

		new (pData) T(*m_pValue);
		return true;
	}

	virtual const TypeInfo& GetTypeInfo() const SEOUL_OVERRIDE
	{
		return TypeId<T>();
	}

	virtual WeakAny GetWeakAny() const SEOUL_OVERRIDE
	{
		return WeakAny(GetValue());
	}

	virtual WeakAny GetWeakAnyPointerToValue() const SEOUL_OVERRIDE
	{
		return WeakAny(&const_cast<T&>(GetValue()));
	}

	virtual WeakAny GetWeakAnyConstPointerToValue() const SEOUL_OVERRIDE
	{
		return WeakAny(&const_cast<T const&>(GetValue()));
	}

	T& GetValue() const
	{
		return *m_pValue;
	}

	virtual WeakAny GetPointerToObject() const SEOUL_OVERRIDE
	{
		return (m_pValue.Get());
	}

	virtual void const* GetConstVoidStarPointerToObject() const SEOUL_OVERRIDE
	{
		return reinterpret_cast<void const*>(m_pValue.Get());
	}

private:
	ScopedPtr<T> m_pValue;

	SEOUL_DISABLE_COPY(Placeholder);
}; // struct Placeholder

/**
 * Specialization of Placeholder for handling specialization on void.
 */
template <>
struct Placeholder<void, false> SEOUL_SEALED : public BasePlaceholder
{
	Placeholder()
	{
	}

	virtual void Clone(Byte aInternalValue[SEOUL_ANY_INTERNAL_STORAGE_SIZE]) const SEOUL_OVERRIDE
	{
		SEOUL_STATIC_ASSERT(sizeof(Placeholder<void, false>) <= SEOUL_ANY_INTERNAL_STORAGE_SIZE);
		new (reinterpret_cast<Placeholder<void, false>*>(aInternalValue)) Placeholder<void, false>;
	}

	virtual bool Clone(void* pData, size_t zDataSizeInBytes) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual WeakAny GetWeakAny() const SEOUL_OVERRIDE
	{
		return WeakAny();
	}

	virtual WeakAny GetWeakAnyPointerToValue() const SEOUL_OVERRIDE
	{
		return WeakAny();
	}

	virtual WeakAny GetWeakAnyConstPointerToValue() const SEOUL_OVERRIDE
	{
		return WeakAny();
	}

	virtual const TypeInfo& GetTypeInfo() const SEOUL_OVERRIDE
	{
		return TypeId<void>();
	}

	virtual WeakAny GetPointerToObject() const SEOUL_OVERRIDE
	{
		return WeakAny();
	}

	virtual void const* GetConstVoidStarPointerToObject() const SEOUL_OVERRIDE
	{
		return nullptr;
	}

private:
	SEOUL_DISABLE_COPY(Placeholder);
}; // struct Placeholder

} // namespace AnyDetail

/**
 * Any is a concrete class that can store a value of any type. Values that
 * fit within Any's internal storage (in consideration of alignment and the
 * size of a vtable pointer) are copied into the Any object itself. Otherwise,
 * the copy is stored in heap allocated memory.
 *
 * All values stored in an Any object must be copy constructable.
 */
class Any SEOUL_SEALED
{
public:
	Any()
	{
		InternalInitializeToVoid();
	}

	template <typename T>
	Any(const T& value)
	{
		InternalInitializeTo<T>(value);
	}

	Any(const Any& b)
	{
		if (this == &b)
		{
			return;
		}

		memset(m_aInternalValue, 0, sizeof(m_aInternalValue));
		b.InternalGetBasePlaceholder().Clone(m_aInternalValue);
	}

	template <typename T>
	Any& operator=(const T& value)
	{
		Any(value).Swap(*this);
		return *this;
	}

	Any& operator=(const Any& b)
	{
		if (this == &b)
		{
			return *this;
		}

		InternalGetBasePlaceholder().~BasePlaceholder();
		memset(m_aInternalValue, 0, sizeof(m_aInternalValue));
		b.InternalGetBasePlaceholder().Clone(m_aInternalValue);

		return *this;
	}

	~Any()
	{
		InternalGetBasePlaceholder().~BasePlaceholder();
	}

	Bool CloneTo(void* pData, size_t zDataSizeInBytes) const
	{
		return InternalGetBasePlaceholder().Clone(pData, zDataSizeInBytes);
	}

	/**
	 * @return The reflection TypeInfo object that describes
	 * the type of object contained within this Any.
	 *
	 * If this Any has not been initialized to
	 * an explicit value, this method will returns the TypeInfo
	 * of void.
	 */
	const TypeInfo& GetTypeInfo() const
	{
		return InternalGetBasePlaceholder().GetTypeInfo();
	}

	/**
	 * @return A WeakAny to the data contain in this Any.
	 */
	WeakAny GetWeakAny() const
	{
		return InternalGetBasePlaceholder().GetWeakAny();
	}

	/**
	 * @return A WeakAny that contains a writeable pointer to the data contained in this Any.
	 */
	WeakAny GetWeakAnyPointerToValue() const
	{
		return InternalGetBasePlaceholder().GetWeakAnyPointerToValue();
	}

	/**
	 * @return A WeakAny that contains a writeable pointer to the data contained in this Any.
	 */
	WeakAny GetWeakAnyConstPointerToValue() const
	{
		return InternalGetBasePlaceholder().GetWeakAnyConstPointerToValue();
	}

	/**
	 * @return The Reflection::Type of this WeakAny, this is a helper
	 * method, equivalent to GetTypeInfo().GetType().
	 */
	const Reflection::Type& GetType() const
	{
		return GetTypeInfo().GetType();
	}

	/**
	 * @return True if this Any is not set to any object, false otherwise.
	 */
	Bool IsValid() const
	{
		return !IsOfType<void>();
	}

	/**
	 * @return True if the data contained within this Any
	 * is of type T, false otherwise.
	 */
	template <typename U>
	Bool IsOfType() const
	{
		typedef typename RemoveConstVolatileReference<U>::Type Type;

		return (TypeId<Type>() == GetTypeInfo());
	}

	/**
	 * Reset this Any to the invalid state.
	 */
	void Reset()
	{
		Any any;
		Swap(any);
	}

	/**
	 * Swap the data contained within this Any with
	 * the data in b.
	 */
	void Swap(Any& b)
	{
		Seoul::Swap(*this, b);
	}

	/**
	 * @return The data contained within this Any to
	 * type T.
	 *
	 * \warning The data must be of type T or this method will
	 * SEOUL_FAIL(). Use IsOfType() to check the type first.
	 */
	template <typename U>
	typename RemoveConstVolatileReference<U>::Type& Cast() const
	{
		typedef typename RemoveConstVolatileReference<U>::Type Type;
		typedef AnyDetail::Placeholder<Type, SEOUL_ANY_IN_PLACE(Type)> PlaceholderType;
		SEOUL_STATIC_ASSERT(sizeof(PlaceholderType) <= sizeof(m_aInternalValue));
		SEOUL_STATIC_ASSERT(
			SEOUL_ALIGN_OF(PlaceholderType) <= SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT &&
			0 == SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT % SEOUL_ALIGN_OF(PlaceholderType));

		SEOUL_ASSERT(IsOfType<Type>());
		PlaceholderType const* pPlaceholder = static_cast< PlaceholderType const* >(
			&InternalGetBasePlaceholder());
		SEOUL_ASSERT(pPlaceholder);

		return pPlaceholder->GetValue();
	}

	/**
	 * @return A WeakAny wrapper around a pointer to the object contained in this Any,
	 * or nullptr if this Any does not contain a value.
	 *
	 * The const modified of the pointer will match the contained value - either
	 * const or not const depending on whether the contained value is const or not const.
	 */
	WeakAny GetPointerToObject() const
	{
		return InternalGetBasePlaceholder().GetPointerToObject();
	}

	/**
	 * @return A void const* pointer to the object contained in this Any, or nullptr
	 * if this Any does not contain a value.
	 */
	void const* GetConstVoidStarPointerToObject() const
	{
		return InternalGetBasePlaceholder().GetConstVoidStarPointerToObject();
	}

private:
	AnyDetail::BasePlaceholder const& InternalGetBasePlaceholder() const
	{
		return *reinterpret_cast<AnyDetail::BasePlaceholder const*>(m_aInternalValue);
	}

	AnyDetail::BasePlaceholder& InternalGetBasePlaceholder()
	{
		return *reinterpret_cast<AnyDetail::BasePlaceholder*>(m_aInternalValue);
	}

	void InternalInitializeToVoid()
	{
		memset(m_aInternalValue, 0, sizeof(m_aInternalValue));
		new (reinterpret_cast<AnyDetail::Placeholder<void, false>*>(m_aInternalValue)) AnyDetail::Placeholder<void, false>();
	}

	template <typename U>
	void InternalInitializeTo(const U& v)
	{
		typedef typename RemoveConstVolatileReference<U>::Type Type;
		typedef AnyDetail::Placeholder<Type, SEOUL_ANY_IN_PLACE(Type)> PlaceholderType;

		SEOUL_STATIC_ASSERT(sizeof(PlaceholderType) <= sizeof(m_aInternalValue));
		SEOUL_STATIC_ASSERT(
			SEOUL_ALIGN_OF(PlaceholderType) <= SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT &&
			0 == SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT % SEOUL_ALIGN_OF(PlaceholderType));

		memset(m_aInternalValue, 0, sizeof(m_aInternalValue));
		new (reinterpret_cast<PlaceholderType*>(m_aInternalValue)) PlaceholderType(v);
	}

	SEOUL_DECLARE_ALIGNMENT_TYPE(Byte, SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT) m_aInternalValue[SEOUL_ANY_INTERNAL_STORAGE_SIZE];
}; // class Any

// Sanity checks that we got the alignment and size of the Any type and its
// placeholder internal class right.
SEOUL_STATIC_ASSERT(sizeof(Any) == SEOUL_ANY_INTERNAL_STORAGE_SIZE);
SEOUL_STATIC_ASSERT(SEOUL_ALIGN_OF(Any) == SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT);

#undef SEOUL_ANY_IN_PLACE
#undef SEOUL_ANY_LARGEST_TYPE_SIZE
#undef SEOUL_ANY_INTERNAL_STORAGE_SIZE
#undef SEOUL_ANY_INTERNAL_STORAGE_ALIGNMENT

/**
 * Construct this WeakAny from the type in an Any b.
 */
inline WeakAny::WeakAny(const Any& b)
{
	*this = b.GetWeakAny();
}

} // namespace Seoul::Reflection

#endif // include guard
