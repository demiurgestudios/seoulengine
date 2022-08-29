/**
 * \file ReflectionProperty.cpp
 * \brief Reflection object used to define a reflectable property of
 * a reflectable class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionAttribute.h"
#include "ReflectionProperty.h"

namespace Seoul::Reflection
{

Property::Property(
	HString name,
	const TypeInfo& memberTypeInfo,
	TryGetFunc tryGet,
	TrySetFunc trySet,
	TryGetPtrFunc tryGetPtr,
	TryGetConstPtrFunc tryGetConstPtr,
	UInt16 uFlags /*= 0u */,
	ptrdiff_t iOffset /* = -1 */)
	: m_pClassTypeInfo(nullptr)
	, m_MemberTypeInfo(memberTypeInfo)
	, m_TryGet(tryGet)
	, m_TrySet(trySet)
	, m_TryGetPtr(tryGetPtr)
	, m_TryGetConstPtr(tryGetConstPtr)
	, m_Name(name)
	, m_uFlags(uFlags)
	, m_iOffset(iOffset)
{
}

Property::~Property()
{
	m_Attributes.DestroyAttributes();
}

} // namespace Seoul::Reflection
