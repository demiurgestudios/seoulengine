/**
 * \file ReflectionSimpleCast.h
 * \brief Handles conversion of one of the any types
 * (WeakAny and Any) from simple types to concrete types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_SIMPLE_CAST_H
#define REFLECTION_SIMPLE_CAST_H

#include "Prereqs.h"

namespace Seoul::Reflection
{

template <typename ANY, typename FROM, typename TO, Bool IS_CONVERTIBLE>
struct SimpleCastUtil
{
	static inline Bool Cast(const ANY& any, TO& r)
	{
		return false;
	}
}; // struct SimpleCastUtil

template <typename ANY, typename FROM, typename TO>
struct SimpleCastUtil<ANY, FROM, TO, true>
{
	static inline Bool Cast(const ANY& any, TO& r)
	{
		r = TO(any.template Cast<FROM>());
		return true;
	}
}; // struct SimpleCastUtil

template <typename ANY, typename FROM>
struct SimpleCastUtil<ANY, FROM, Bool, true>
{
	static inline Bool Cast(const ANY& any, Bool& r)
	{
		r = (Bool)(!!any.template Cast<FROM>());
		return true;
	}
}; // struct SimpleCastUtil

template <typename ANY, typename TO, Bool IS_CONVERTIBLE>
struct SimpleCastEnumUtil
{
	static inline Bool Cast(const ANY& any, TO& r)
	{
		return false;
	}
}; // struct SimpleCastEnumUtil

template <typename ANY, typename TO>
struct SimpleCastEnumUtil<ANY, TO, true>
{
	static inline Bool Cast(const ANY& any, TO& r)
	{
		if (IsConvertible<Int, TO>::Value)
		{
			auto const& typeInfo = any.GetTypeInfo();
			SEOUL_ASSERT(typeInfo.GetSizeInBytes() <= sizeof(Int));
			Int iValue = 0;
			memcpy(&iValue, any.GetConstVoidStarPointerToObject(), typeInfo.GetSizeInBytes());
			r = TO(iValue);
			return true;
		}
		return false;
	}
}; // struct SimpleCastEnumUtil

template <typename ANY>
struct SimpleCastEnumUtil<ANY, Bool, true>
{
	static inline Bool Cast(const ANY& any, Bool& r)
	{
		auto const& typeInfo = any.GetTypeInfo();
		SEOUL_ASSERT(typeInfo.GetSizeInBytes() <= sizeof(Int));
		Int iValue = 0;
		memcpy(&iValue, any.GetConstVoidStarPointerToObject(), typeInfo.GetSizeInBytes());
		r = (Bool)(!!iValue);
		return true;
	}
}; // struct SimpleCastEnumUtil

#define SEOUL_SIMPLE_CAST(from) \
	SimpleCastUtil<ANY, from, T, IsConvertible<from, T>::Value>::Cast(any, r)

template <typename ANY, typename T>
static inline Bool SimpleCast(const ANY& any, T& r)
{
	auto const& typeInfo = any.GetTypeInfo();
	switch (typeInfo.GetSimpleTypeInfo())
	{
	case SimpleTypeInfo::kBoolean:
		return SEOUL_SIMPLE_CAST(Bool);
	case SimpleTypeInfo::kCString:
		return SEOUL_SIMPLE_CAST(Byte const*);
	case SimpleTypeInfo::kEnum:
		return SimpleCastEnumUtil<ANY, T, IsConvertible<Int, T>::Value>::Cast(any, r);
	case SimpleTypeInfo::kInt8:
		return SEOUL_SIMPLE_CAST(Int8);
	case SimpleTypeInfo::kInt16:
		return SEOUL_SIMPLE_CAST(Int16);
	case SimpleTypeInfo::kInt32:
		return SEOUL_SIMPLE_CAST(Int32);
	case SimpleTypeInfo::kInt64:
		return SEOUL_SIMPLE_CAST(Int64);
	case SimpleTypeInfo::kFloat32:
		return SEOUL_SIMPLE_CAST(Float32);
	case SimpleTypeInfo::kFloat64:
		return SEOUL_SIMPLE_CAST(Float64);
	case SimpleTypeInfo::kHString:
		return SEOUL_SIMPLE_CAST(HString);
	case SimpleTypeInfo::kString:
		return SEOUL_SIMPLE_CAST(String);
	case SimpleTypeInfo::kUInt8:
		return SEOUL_SIMPLE_CAST(UInt8);
	case SimpleTypeInfo::kUInt16:
		return SEOUL_SIMPLE_CAST(UInt16);
	case SimpleTypeInfo::kUInt32:
		return SEOUL_SIMPLE_CAST(UInt32);
	case SimpleTypeInfo::kUInt64:
		return SEOUL_SIMPLE_CAST(UInt64);

	case SimpleTypeInfo::kComplex: // fall-through
	default:
		break;
	};

	return false;
}

#undef SEOUL_SIMPLE_CAST

} // namespace Seoul::Reflection

#endif // include guard
