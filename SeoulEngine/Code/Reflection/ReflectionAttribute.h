/**
 * \file ReflectionAttribute.h
 * \brief Attribute is the base class for
 * all attributes that can be attached to reflection definitions. Attributes
 * are metadata - they allow various traits to be associated with a member
 * property, method, or type that can then be used for a wide variety
 * of queries.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_ATTRIBUTE_H
#define REFLECTION_ATTRIBUTE_H

#include "ReflectionPrereqs.h"
#include "SeoulHString.h"

namespace Seoul::Reflection
{

/**
 * Base class for reflection attributes. Attributes are added
 * to reflection definitions to specify the definition - for example,
 * adding an Attribute::Description() attribute allows a human
 * readable description to be added to a class, property, etc. reflection
 * definition.
 */
class Attribute SEOUL_ABSTRACT
{
public:
	virtual ~Attribute()
	{
	}

	/**
	 * For attributes attached to methods, this specifies
	 * the method argument that the attribute is attached to.
	 * -1 indicates a non-method attribute, or a method attribute
	 * that is defined on the method itself, not any of its arguments.
	 */
	Int GetArg() const { return m_iArg; }
	Attribute* SetArg(Int iArg) { m_iArg = iArg; return this; }

	/**
	 * Return the id of this attribute - all attributes with
	 * the same Id are considered to be of the same type. Therefore,
	 * for two attributes to be different types of attributes, they
	 * must have different, unique ids.
	 */
	virtual HString GetId() const = 0;

protected:
	// Can only be constructed by a derived class.
	Attribute()
		: m_iArg(-1)
	{
	}

private:
	Int m_iArg;

	SEOUL_DISABLE_COPY(Attribute);
}; // class Attribute

/**
 * Set of attributes - usually used to store all the attributes associated with a type.
 */
class AttributeCollection SEOUL_SEALED
{
public:
	AttributeCollection()
		: m_vAttributes()
	{
	}

	AttributeCollection(const AttributeCollection& b)
		: m_vAttributes(b.m_vAttributes)
	{
	}

	~AttributeCollection()
	{
	}

	void AddAttribute(Attribute* pAttribute)
	{
		SEOUL_ASSERT(nullptr != pAttribute);
		m_vAttributes.PushBack(pAttribute);
	}

	Attribute const* GetAttribute(HString id, Int iArg = -1) const;

	/**
	 * @return The attribute of type T in this AttributeCollection, or nullptr if no
	 * such attribute is assigned.
	 */
	template <typename T>
	T const* GetAttribute(Int iArg = -1) const
	{
		return static_cast<T const*>(GetAttribute(T::StaticId(), iArg));
	}

	/**
	 * @return The total number of attributes in this AttributeCollection.
	 */
	UInt32 GetCount() const
	{
		return m_vAttributes.GetSize();
	}

	/**
	 * @return True if the attribute id is in this AttributeCollection.
	 */
	Bool HasAttribute(HString id) const
	{
		return (nullptr != GetAttribute(id));
	}

	/**
	 * @return True if the attribute type T is in this AttributeCollection.
	 */
	template <typename T>
	Bool HasAttribute() const
	{
		return HasAttribute(T::StaticId());
	}

	void Swap(AttributeCollection& rB)
	{
		m_vAttributes.Swap(rB.m_vAttributes);
	}

	const AttributeVector& GetAttributeVector() const
	{
		return m_vAttributes;
	}

private:
	void DestroyAttributes()
	{
		SafeDeleteVector(m_vAttributes);
	}

	friend class Method;
	friend class Property;
	friend class Type;

	AttributeVector m_vAttributes;
};
typedef Vector< AttributeCollection, MemoryBudgets::Reflection> EnumAttributeVector;

} // namespace Seoul::Reflection

#endif // include guard
