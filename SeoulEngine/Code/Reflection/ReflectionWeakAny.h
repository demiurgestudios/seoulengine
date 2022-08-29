/**
 * \file ReflectionWeakAny.h
 * \brief WeakAny is a concrete class that can wrap any type. Unlike Any,
 * WeakAny does not guarantee that a copy of the source object is always
 * made, and may store just a pointer to the object. As a result, WeakAny
 * will usually have a much lower computational cost but it is also unsafe
 * to use in contexts where the lifespan of the source object does not
 * span the lifespan of the WeakAny object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_WEAK_ANY_H
#define REFLECTION_WEAK_ANY_H

#include "Algorithms.h"
#include "Prereqs.h"
#include "ReflectionTypeInfo.h"
#include "ScopedPtr.h"

namespace Seoul::Reflection
{

// Forward declarations
class Any;

/**
 * Used to reference data of any type.
 * Allows for safe access to the stored data - use IsOfType()
 * with a concrete template argument to check if the stored data
 * is of the type you expect.
 *
 * \warning WeakAny should only be used for passing data of any
 * type, not storing it.
 */
class WeakAny SEOUL_SEALED
{
public:
	WeakAny()
		: m_pTypeInfo(nullptr)
		, m_pData(nullptr)
	{
	}

	template <typename T>
	WeakAny(const T& value)
		: m_pTypeInfo(nullptr)
		, m_pData(nullptr)
	{
		typedef typename RemoveConstVolatileReference<T>::Type Type;

		m_pTypeInfo = &TypeId<Type>();

		if (UsesInlineStorage())
		{
			new (&m_pData) Type(value);
		}
		else
		{
			m_pData = (Type*)&value;
		}
	}

	WeakAny(const Any& b);

	WeakAny(const WeakAny& b)
		: m_pTypeInfo(b.m_pTypeInfo)
		, m_pData(b.m_pData)
	{
	}

	template <typename T>
	WeakAny& operator=(const T& value)
	{
		WeakAny(value).Swap(*this);
		return *this;
	}

	WeakAny& operator=(const WeakAny& b)
	{
		m_pTypeInfo = b.m_pTypeInfo;
		m_pData = b.m_pData;
		return *this;
	}

	/**
	 * @return The reflection TypeInfo object that describes
	 * the type of object referenced by this WeakAny.
	 *
	 * If this WeakAny has not been initialized to
	 * an explicit value, this method will returns the TypeInfo
	 * of void.
	 */
	const TypeInfo& GetTypeInfo() const
	{
		return (nullptr == m_pTypeInfo ? TypeId<void>() : *m_pTypeInfo);
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
	 * @return True if this WeakAny is not set to any object, false otherwise.
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
	 * Reset this WeakAny to the invalid state.
	 */
	void Reset()
	{
		WeakAny any;
		Swap(any);
	}

	/**
	 * Swap the data contained within this Any with
	 * the data in b.
	 */
	void Swap(WeakAny& b)
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
	const typename RemoveConstVolatileReference<U>::Type& Cast() const
	{
		typedef typename RemoveConstVolatileReference<U>::Type Type;

		SEOUL_ASSERT(IsOfType<Type>());
		return *((Type*)GetConstVoidStarPointerToObject());
	}

	/**
	 * @return A void const* pointer to the object contained in this Any, or nullptr
	 * if this Any does not contain a value.
	 */
	void const* GetConstVoidStarPointerToObject() const
	{
		if (UsesInlineStorage())
		{
			return &m_pData;
		}
		else
		{
			return m_pData;
		}
	}

private:
	Reflection::TypeInfo const* m_pTypeInfo;
	void* m_pData;

	Bool UsesInlineStorage() const
	{
		const TypeInfo& info = GetTypeInfo();
		return (info.GetSizeInBytes() <= sizeof(m_pData)
			&& (info.GetSimpleTypeInfo() != SimpleTypeInfo::kComplex
				|| info.IsPointer()));
	}
}; // class WeakAny

} // namespace Seoul::Reflection

#endif // include guard
