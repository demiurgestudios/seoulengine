/**
 * \file ReflectionRegistry.cpp
 * \brief The Registry is the one and only global collection of runtime reflection
 * Type objects. It is used primarily to get Type objects by identifier name.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionRegistry.h"
#include "ReflectionType.h"

namespace Seoul::Reflection
{

Registry::~Registry()
{
}

/**
 * @return The global Registry singleton.
 */
const Registry& Registry::GetRegistry()
{
	return GetSingleton();
}

Registry& Registry::GetSingleton()
{
	static Registry s_Registry;
	return s_Registry;
}

Registry::Registry()
{
}

Bool Registry::AddType(Type const& rType, UInt32& ruRegistryIndex)
{
	UInt16 uIndex = (UInt16)m_vTypes.GetSize();
	if (m_tTypes.Insert(rType.GetName(), uIndex).Second)
	{
		m_vTypes.PushBack(&rType);
		ruRegistryIndex = uIndex;
		return true;
	}

	return false;
}

} // namespace Seoul::Reflection
