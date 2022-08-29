/**
 * \file ReflectionAttributes.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionAttributes.h"
#include "ReflectionType.h"
#include "ReflectionTypeInfo.h"
#include "SeoulMath.h"

namespace Seoul::Reflection
{

namespace Attributes
{

Bool DoNotSerializeIfEqualToSimpleType::Equals(const Property& property, const WeakAny& objectThis, SerializeError& error) const
{
	error = SerializeError::kNone;

	Any givenAnyValue;
	if (!property.TryGet(objectThis, givenAnyValue))
	{
		return false;
	}

	const TypeInfo& expectedTypeInfo = m_Value.GetTypeInfo();
	const TypeInfo& givenTypeInfo = givenAnyValue.GetTypeInfo();
	auto expectedSimpleTypeInfo = expectedTypeInfo.GetSimpleTypeInfo();
	auto givenSimpleTypeInfo = givenTypeInfo.GetSimpleTypeInfo();

	if (givenSimpleTypeInfo != expectedSimpleTypeInfo)
	{
		error = SerializeError::kDoNotSerializeIfEqualToSimpleTypeTypeMismatch;
		return false;
	}

	switch(expectedSimpleTypeInfo)
	{
		case SimpleTypeInfo::kBoolean:
		{
			Bool bGivenValue = false, bExpectedValue = false;
			return (
				TypeConstruct(givenAnyValue, bGivenValue)
				&& TypeConstruct(m_Value, bExpectedValue)
				&& bGivenValue == bExpectedValue);
		}
		break;

		case SimpleTypeInfo::kCString:
		{
			Byte const* sGivenString = nullptr;
			Byte const* sExpectedString = nullptr;
			if (TypeConstruct(givenAnyValue, sGivenString)
				&& TypeConstruct(m_Value, sExpectedString))
			{
				if (nullptr == sGivenString || nullptr == sExpectedString)
				{
					return sGivenString == sExpectedString;
				}
				return strcmp(sGivenString, sExpectedString) == 0;
			}
			return false;
		}
		break;

		case SimpleTypeInfo::kEnum:
		{
			HString givenEnumName, expectedEnumName;
			return (
				givenAnyValue.GetType().TryGetEnum()->TryGetName(givenAnyValue, givenEnumName)
				&& m_Value.GetType().TryGetEnum()->TryGetName(m_Value, expectedEnumName)
				&& givenEnumName == expectedEnumName);
		}
		break;

		case SimpleTypeInfo::kFloat32: // fall-through
		case SimpleTypeInfo::kFloat64:
		{
			Float64 fGivenValue = 0.0, fExpectedValue = 0.0;
			return (
				TypeConstruct(givenAnyValue, fGivenValue)
				&& TypeConstruct(m_Value, fExpectedValue)
				&& Seoul::Equals(fGivenValue, fExpectedValue));
		}
		break;

		case SimpleTypeInfo::kHString:
		{
			HString givenIdentifier, expectedIdentifier;
			return (
				TypeConstruct(givenAnyValue, givenIdentifier)
				&& TypeConstruct(m_Value, expectedIdentifier)
				&& givenIdentifier == expectedIdentifier);
		}
		break;

		case SimpleTypeInfo::kString:
		{
			String givenValue, expectedValue;
			return (
				TypeConstruct(givenAnyValue, givenValue)
				&& TypeConstruct(m_Value, expectedValue)
				&& givenValue == expectedValue);
		}
		break;

		// Integral types except for UInt64 can be treated as an Int64.
		case SimpleTypeInfo::kInt8: // fall-through
		case SimpleTypeInfo::kInt16: // fall-through
		case SimpleTypeInfo::kInt32: // fall-through
		case SimpleTypeInfo::kInt64: // fall-through
		case SimpleTypeInfo::kUInt8: // fall-through
		case SimpleTypeInfo::kUInt16: // fall-through
		case SimpleTypeInfo::kUInt32:
		{
			Int64 givenValue, expectedValue = 0;
			return (
				TypeConstruct(givenAnyValue, givenValue)
				&& TypeConstruct(m_Value, expectedValue)
				&& givenValue == expectedValue);
		}
		break;

		case SimpleTypeInfo::kUInt64:
		{
			UInt64 givenValue, expectedValue = 0u;
			return (
				TypeConstruct(givenAnyValue, givenValue)
				&& TypeConstruct(m_Value, expectedValue)
				&& givenValue == expectedValue);
		}
		break;

		case SimpleTypeInfo::kComplex:
		{
			error = SerializeError::kDoNotSerializeIfEqualToSimpleTypeComplexTypeGiven;
			return false;
		}
		default:
		return false;
	};
}

} // namespace Attributes

} // namespace Seoul::Reflection
