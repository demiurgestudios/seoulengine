/**
 * \file NamedType.h
 * \brief Defines a boxed wrapper type for utilizing type checker safety
 * to disambiguate between identical types of different sorts (e.g.
 * a FooId and a BarId that are both a UInt32).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NAMED_TYPE_H
#define NAMED_TYPE_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Tag is a unique type used to uniqueify the NamedType.
 */
template <typename T, typename Tag>
class NamedType
{
public:
	NamedType()
		: m_Value()
	{
	}

	explicit NamedType(const T& t)
		: m_Value(t)
	{
	}

	explicit operator T() const
	{ 
		return m_Value;
	}

	Bool operator==(const NamedType<T, Tag> & other) const
	{
		return m_Value == other.m_Value;
	}

	NamedType<T, Tag>& operator=(const NamedType<T, Tag> & other)
	{
		m_Value = other.m_Value;
		return *this;
	}

	Bool operator!=(const NamedType<T, Tag> & other) const
	{
		return m_Value != other.m_Value;
	}
	
	const T& GetValue() const { return m_Value; }
	T& GetValue() { return m_Value; }

	// if you're adding additional functions, please update tests
	// in NamedTypeTest and ReflectionTest

private:
	T m_Value;
}; // class NamedType

} // namespace Seoul

#endif // include guard
