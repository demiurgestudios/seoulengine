/**
 * \file ReflectionPropertyDetail.h
 * \brief Internal mechanics of constructing a Property object
 * for a class member variable. Unless you're modifying the
 * Reflection system, you will likely not need to use
 * this class directly.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_PROPERTY_DETAIL_H
#define REFLECTION_PROPERTY_DETAIL_H

#include "Prereqs.h"
#include "ReflectionPrereqs.h"
#include "ReflectionProperty.h"
#include "ReflectionType.h"
#include "ReflectionWeakAny.h"
#include "ScopedPtr.h"
#include "SeoulHString.h"

namespace Seoul::Reflection
{

// Forward declarations
class Attribute;
class Type;

namespace PropertyDetail
{

	// Implementation Property classes for each variation - see
	// ReflectionPropertyVariations.h for the variations currently implemented,
	// and ReflectionPropertyDetailInternal.h for the template that is used
	// to implement the Property class for each variation.
#	define SEOUL_PROPERTY_VARIATION_IMPL_FILENAME "ReflectionPropertyDetailInternal.h"
#	include "ReflectionPropertyVariations.h"

	struct TypicalFieldPropertyDetail
	{
		template <typename C, typename T>
		static Bool TryGet(const Property& prop, const WeakAny& thisPointer, Any& rValue)
		{
			auto const iOffset = prop.GetOffset();
			C const* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				SEOUL_ASSERT(nullptr != p);

				// Use the raw offset to adjust the converted pointer to the member.
				// Offset was computed from a C pointer.
				rValue = *((T*)((Byte*)p + iOffset));
				return true;
			}

			return false;
		}

		template <typename C, typename T>
		static Bool TrySet(const Property& prop, const WeakAny& thisPointer, const WeakAny& value)
		{
			auto const iOffset = prop.GetOffset();
			C* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				SEOUL_ASSERT(nullptr != p);
				typename RemoveConst< typename RemoveReference<T>::Type >::Type val;
				if (TypeConstruct(value, val))
				{
					// Use the raw offset to adjust the converted pointer to the member.
					// Offset was computed from a C pointer.
					*((T*)((Byte*)p + iOffset)) = val;
					return true;
				}
			}

			return false;
		}

		template <typename C, typename T>
		static Bool TryGetPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			auto const iOffset = prop.GetOffset();
			C* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				// Use the raw offset to adjust the converted pointer to the member.
				// Offset was computed from a C pointer.
				SEOUL_ASSERT(nullptr != p);
				rValue = (T*)((Byte*)p + iOffset);
				return true;
			}

			return false;
		}

		template <typename C, typename T>
		static Bool TryGetConstPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			auto const iOffset = prop.GetOffset();
			C const* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				// Use the raw offset to adjust the converted pointer to the member.
				// Offset was computed from a C pointer.
				SEOUL_ASSERT(nullptr != p);
				rValue = (T const*)((Byte*)p + iOffset);
				return true;
			}

			return false;
		}
	};

	struct TypicalStaticFieldPropertyDetail
	{
		template <typename T>
		static Bool TryGet(const Property& prop, const WeakAny& thisPointer, Any& rValue)
		{
			rValue = *((T*)((size_t)prop.GetOffset()));
			return true;
		}

		template <typename T>
		static Bool TrySet(const Property& prop, const WeakAny& thisPointer, const WeakAny& value)
		{
			typename RemoveConst< typename RemoveReference<T>::Type >::Type val;
			if (TypeConstruct(value, val))
			{
				*((T*)((size_t)prop.GetOffset())) = val;
				return true;
			}

			return false;
		}

		template <typename T>
		static Bool TryGetPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			rValue = (T*)((size_t)prop.GetOffset());
			return true;
		}

		template <typename T>
		static Bool TryGetConstPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			rValue = (T const*)((size_t)prop.GetOffset());
			return true;
		}
	};

	/**
	 * Specialization of property handling for the most common case (simple read/write field property
	 * with no flags). Designed to improve compilation times by significantly reducing the overhead
	 * of compilation for this case (no complex class specialization to discover behavior for this
	 * case, and the function specializations are keyed on class+template instead of the pointer-to-member,
	 * reducing the number of variations.
	 *
	 * Note that this could be further improve by decoupling C and T in the specializations above, but
	 * this did not appear to provide a significant enough benefit for the additional complexity (and
	 * possible additional runtime cost).
	 */
	template <typename C, typename T>
	Property* MakeTypicalFieldProperty(HString name, T(C::*prop))
	{
		return SEOUL_NEW(MemoryBudgets::Reflection) Property(
			name,
			TypeInfoDetail::TypeInfoImpl<T>::Get(),
			TypicalFieldPropertyDetail::TryGet<C, T>,
			TypicalFieldPropertyDetail::TrySet<C, T>,
			TypicalFieldPropertyDetail::TryGetPtr<C, T>,
			TypicalFieldPropertyDetail::TryGetConstPtr<C, T>,
			0u, /* always read-write, no flags */
			((size_t)&(((C*)nullptr)->*prop))); /* manual version of the offsetof() macro - we are computing offset in bytes from a C* pointer to the prop member. */
	}

	/** Same as typical field but for static members. */
	template <typename T>
	Property* MakeTypicalFieldProperty(HString name, T* prop)
	{
		return SEOUL_NEW(MemoryBudgets::Reflection) Property(
			name,
			TypeInfoDetail::TypeInfoImpl<T>::Get(),
			TypicalStaticFieldPropertyDetail::TryGet<T>,
			TypicalStaticFieldPropertyDetail::TrySet<T>,
			TypicalStaticFieldPropertyDetail::TryGetPtr<T>,
			TypicalStaticFieldPropertyDetail::TryGetConstPtr<T>,
			PropertyFlags::kIsStatic, /* always read-write, no flags */
			(size_t)prop);
	}

} // namespace PropertyDetail

} // namespace Seoul::Reflection

#endif // include guard
