/**
 * \file ReflectionProperty.h
 * \brief Reflection object used to define a reflectable property of
 * a reflectable class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_PROPERTY_H
#define REFLECTION_PROPERTY_H

#include "Prereqs.h"
#include "ReflectionAttribute.h"
namespace Seoul { namespace Reflection { class Any; } }
namespace Seoul { namespace Reflection { class Type; } }
namespace Seoul { namespace Reflection { class WeakAny; } }

namespace Seoul::Reflection
{

namespace PropertyFlags
{
	/**
	 * Describe various access privileges of the property - if none of these flags are set,
	 * the property has full access privileges - the property can be used to get, set, and store
	 * the data associated with the property.
	 */
	enum Enum
	{
		kNone = 0,

		/** If set, TryGet() will always return false. */
		kDisableGet = (1 << 0),

		/** If set, TrySet() will always return false. */
		kDisableSet = (1 << 1),

		/** Property is a static member vs. an instance member. */
		kIsStatic = (1 << 2),
	};
}

/**
 * Property implements Get()/Set() semantics of member variables of a class. Under the hood, a property
 * can be exposed via a getter/setter function pair, a pointer-to-member, or some combination thereof. In practice,
 * using a pointer-to-member is the most efficient, and allows the base Property class to directly memcpy() the
 * member variable for some operations.
 *
 * Property is the main ingredient for representing the data of a Type - properties are enumerated when storing
 * a class.
 */
class Property SEOUL_SEALED
{
public:
	typedef Bool (*TryGetFunc)(const Property& prop, const WeakAny& thisPointer, Any& rValue);
	typedef Bool (*TrySetFunc)(const Property& prop, const WeakAny& thisPointer, const WeakAny& value);
	typedef Bool (*TryGetPtrFunc)(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue);
	typedef Bool (*TryGetConstPtrFunc)(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue);

	Property(
		HString name,
		const TypeInfo& memberTypeInfo,
		TryGetFunc tryGet,
		TrySetFunc trySet,
		TryGetPtrFunc tryGetPtr,
		TryGetConstPtrFunc tryGetConstPtr,
		UInt16 uFlags = 0u,
		ptrdiff_t iOffset = -1);
	~Property();

	/**
	 * @return The identifying name of the property - must be unique from all other properties in the owner Type.
	 */
	HString GetName() const
	{
		return m_Name;
	}

	/**
	 * @return The collection of attributes associated with this Property - attributes can be used to classify, categorize,
	 * or otherwise add metadata to a property.
	 */
	const AttributeCollection& GetAttributes() const
	{
		return m_Attributes;
	}

	// Call this method to attempt to get data out of this Property from the instance in thisPointer.
	Bool TryGet(const WeakAny& thisPointer, Any& rValue) const
	{
		return m_TryGet(*this, thisPointer, rValue);
	}

	// Call this method to attempt to set data to this Property, into the instance in thisPointer.
	Bool TrySet(const WeakAny& thisPointer, const WeakAny& value) const
	{
		return m_TrySet(*this, thisPointer, value);
	}

	// Call this method to assign a read-write pointer to rValue.
	Bool TryGetPtr(const WeakAny& thisPointer, WeakAny& rValue) const
	{
		return m_TryGetPtr(*this, thisPointer, rValue);
	}

	// Call this method to assign a read-only pointer to rValue.
	Bool TryGetConstPtr(const WeakAny& thisPointer, WeakAny& rValue) const
	{
		return m_TryGetConstPtr(*this, thisPointer, rValue);
	}
	
	// (Defined in ReflectionType.h)
	// Attempt to assign a pointer to the property data in thisPointer to rpValue.
	template <typename T>
	Bool TryGetPtr(const WeakAny& thisPointer, T*& rpValue) const;

	// Attempt to assign a read-only pointer to the property data in thisPointer to rpValue.
	template <typename T>
	Bool TryGetConstPtr(const WeakAny& thisPointer, T const*& rpValue) const;

	// Return the TypeInfo of the class that this property is associated with.
	const TypeInfo& GetClassTypeInfo() const
	{
		return *m_pClassTypeInfo;
	}

	// Return the TypeInfo of the member type that this property describes.
	const TypeInfo& GetMemberTypeInfo() const
	{
		return m_MemberTypeInfo;
	}

	/** @return True if this Property can get its value, false otherwise. */
	Bool CanGet() const { return (0 == (PropertyFlags::kDisableGet & m_uFlags)); }

	/** @return True if this Property can set its value, false otherwise. */
	Bool CanSet() const { return (0 == (PropertyFlags::kDisableSet & m_uFlags)); }

	/** @return True if this property is a static field vs. instance member. */
	Bool IsStatic() const { return (PropertyFlags::kIsStatic == (PropertyFlags::kIsStatic & m_uFlags)); }

	/** @return Raw offset to the field - for complex properties, undefined, will be -1. */
	ptrdiff_t GetOffset() const
	{
		return m_iOffset;
	}

private:
	AttributeCollection m_Attributes;

	friend struct MethodBuilder;
	friend struct PropertyBuilder;
	friend struct TypeBuilder;
	friend class Type;

	TypeInfo const* m_pClassTypeInfo;
	const TypeInfo& m_MemberTypeInfo;
	TryGetFunc const m_TryGet;
	TrySetFunc const m_TrySet;
	TryGetPtrFunc const m_TryGetPtr;
	TryGetConstPtrFunc const m_TryGetConstPtr;
	HString const m_Name;
	UInt16 const m_uFlags;
	ptrdiff_t const m_iOffset;

	SEOUL_DISABLE_COPY(Property);
}; // class Property

/**
 * @return True if the name of Property a is equal to b.
 */
inline Bool operator==(Property const* a, HString b)
{
	SEOUL_ASSERT(nullptr != a);
	return (a->GetName() == b);
}

/**
 * @return True if the name of Property a is *not* equal to b.
 */
inline Bool operator!=(Property const* a, HString b)
{
	return !(a == b);
}

} // namespace Seoul::Reflection

#endif // include guard
