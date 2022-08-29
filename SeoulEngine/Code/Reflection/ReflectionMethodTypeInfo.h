/**
 * \file ReflectionMethodTypeInfo.h
 * \brief Reflection::MethodTypeInfo is equivalent to TypeInfo, but with
 * additional information for methods (fully defining the signature of the method).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_METHOD_TYPE_INFO_H
#define REFLECTION_METHOD_TYPE_INFO_H

#include "ReflectionPrereqs.h"

namespace Seoul::Reflection
{

// Forward declarations
class Attribute;
class Type;

/**
 * Equivalent to TypeInfo but for methods - note that 2 methods have the same MethodTypeInfo
 * if they have the same fully qualified signature (including const or non-const modifiers). As a result,
 * to fully distinguish a method of one class from a method of another class, you also need the method name
 * of the class and the class type info.
 */
struct MethodTypeInfo SEOUL_SEALED
{
	static const UInt32 kArgumentCount = 15u;

	enum Flags
	{
		kNone = 0,

		/** If set, methods described by this MethodTypeInfo have a const modifier associated with them. */
		kConst = (1 << 0u),

		/** If set, methods described by this MethodTypeInfo are static and can be invoked with an nullptr this pointer. */
		kStatic = (1 << 1u),
	};

	/**
	 * @return True if the method described by this MethodTypeInfo has a const modifier, false otherwise.
	 */
	Bool IsConst() const { return (0u != (m_uFlags & kConst)); }

	/**
	 * @return True if the method described by this MethodTypeInfo is a static method, false otherwise.
	 */
	Bool IsStatic() const { return (0u != (m_uFlags & kStatic)); }

	MethodTypeInfo(
		UInt32 uFlags,
		const TypeInfo& classTypeInfo,
		const TypeInfo& returnTypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument0TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument1TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument2TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument3TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument4TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument5TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument6TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument7TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument8TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument9TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument10TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument11TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument12TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument13TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument14TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get(),
		const TypeInfo& argument15TypeInfo = TypeInfoDetail::TypeInfoImpl<void>::Get())
		: m_uFlags(uFlags)
		, m_rClassTypeInfo(classTypeInfo)
		, m_rReturnValueTypeInfo(returnTypeInfo)
		, m_uArgumentCount(
			(!argument0TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument1TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument2TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument3TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument4TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument5TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument6TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument7TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument8TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument9TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument10TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument11TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument12TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument13TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument14TypeInfo.IsVoid() ? 1u : 0u) +
			(!argument15TypeInfo.IsVoid() ? 1u : 0u))
		, m_rArgument0TypeInfo(argument0TypeInfo)
		, m_rArgument1TypeInfo(argument1TypeInfo)
		, m_rArgument2TypeInfo(argument2TypeInfo)
		, m_rArgument3TypeInfo(argument3TypeInfo)
		, m_rArgument4TypeInfo(argument4TypeInfo)
		, m_rArgument5TypeInfo(argument5TypeInfo)
		, m_rArgument6TypeInfo(argument6TypeInfo)
		, m_rArgument7TypeInfo(argument7TypeInfo)
		, m_rArgument8TypeInfo(argument8TypeInfo)
		, m_rArgument9TypeInfo(argument9TypeInfo)
		, m_rArgument10TypeInfo(argument10TypeInfo)
		, m_rArgument11TypeInfo(argument11TypeInfo)
		, m_rArgument12TypeInfo(argument12TypeInfo)
		, m_rArgument13TypeInfo(argument13TypeInfo)
		, m_rArgument14TypeInfo(argument14TypeInfo)
		, m_rArgument15TypeInfo(argument15TypeInfo)
	{
	}

	MethodTypeInfo(const MethodTypeInfo& b)
		: m_uFlags(b.m_uFlags)
		, m_rClassTypeInfo(b.m_rClassTypeInfo)
		, m_rReturnValueTypeInfo(b.m_rReturnValueTypeInfo)
		, m_uArgumentCount(b.m_uArgumentCount)
		, m_rArgument0TypeInfo(b.m_rArgument0TypeInfo)
		, m_rArgument1TypeInfo(b.m_rArgument1TypeInfo)
		, m_rArgument2TypeInfo(b.m_rArgument2TypeInfo)
		, m_rArgument3TypeInfo(b.m_rArgument3TypeInfo)
		, m_rArgument4TypeInfo(b.m_rArgument4TypeInfo)
		, m_rArgument5TypeInfo(b.m_rArgument5TypeInfo)
		, m_rArgument6TypeInfo(b.m_rArgument6TypeInfo)
		, m_rArgument7TypeInfo(b.m_rArgument7TypeInfo)
		, m_rArgument8TypeInfo(b.m_rArgument8TypeInfo)
		, m_rArgument9TypeInfo(b.m_rArgument9TypeInfo)
		, m_rArgument10TypeInfo(b.m_rArgument10TypeInfo)
		, m_rArgument11TypeInfo(b.m_rArgument11TypeInfo)
		, m_rArgument12TypeInfo(b.m_rArgument12TypeInfo)
		, m_rArgument13TypeInfo(b.m_rArgument13TypeInfo)
		, m_rArgument14TypeInfo(b.m_rArgument14TypeInfo)
		, m_rArgument15TypeInfo(b.m_rArgument15TypeInfo)
	{
	}

	const UInt32 m_uFlags;
	const TypeInfo& m_rClassTypeInfo;
	const TypeInfo& m_rReturnValueTypeInfo;
	const UInt32 m_uArgumentCount;
	const TypeInfo& m_rArgument0TypeInfo;
	const TypeInfo& m_rArgument1TypeInfo;
	const TypeInfo& m_rArgument2TypeInfo;
	const TypeInfo& m_rArgument3TypeInfo;
	const TypeInfo& m_rArgument4TypeInfo;
	const TypeInfo& m_rArgument5TypeInfo;
	const TypeInfo& m_rArgument6TypeInfo;
	const TypeInfo& m_rArgument7TypeInfo;
	const TypeInfo& m_rArgument8TypeInfo;
	const TypeInfo& m_rArgument9TypeInfo;
	const TypeInfo& m_rArgument10TypeInfo;
	const TypeInfo& m_rArgument11TypeInfo;
	const TypeInfo& m_rArgument12TypeInfo;
	const TypeInfo& m_rArgument13TypeInfo;
	const TypeInfo& m_rArgument14TypeInfo;
	const TypeInfo& m_rArgument15TypeInfo;

	const TypeInfo& GetArgumentTypeInfo(UInt32 uIndex) const
	{
		SEOUL_ASSERT(uIndex < kArgumentCount);

		switch (uIndex)
		{
		case 0: return m_rArgument0TypeInfo;
		case 1: return m_rArgument1TypeInfo;
		case 2: return m_rArgument2TypeInfo;
		case 3: return m_rArgument3TypeInfo;
		case 4: return m_rArgument4TypeInfo;
		case 5: return m_rArgument5TypeInfo;
		case 6: return m_rArgument6TypeInfo;
		case 7: return m_rArgument7TypeInfo;
		case 8: return m_rArgument8TypeInfo;
		case 9: return m_rArgument9TypeInfo;
		case 10: return m_rArgument10TypeInfo;
		case 11: return m_rArgument11TypeInfo;
		case 12: return m_rArgument12TypeInfo;
		case 13: return m_rArgument13TypeInfo;
		case 14: return m_rArgument14TypeInfo;
		case 15: return m_rArgument15TypeInfo;
		default:
			SEOUL_FAIL("Out of range value.");
			return m_rArgument0TypeInfo;
		};
	}
};

// The purpose of this assert is to remind you to add another member to the
// MethodTypeInfo struct above when you increase the size of the
// MethodArguments array.  Once you have done so, you can update this assert
// accordingly.
SEOUL_STATIC_ASSERT(MethodArguments::StaticSize == MethodTypeInfo::kArgumentCount);

} // namespace Seoul::Reflection

#endif // include guard
