/**
 * \file ReflectionAttribute.cpp
 * \brief ReflectionAttribute is the base class for
 * all attributes that can be attached to reflection definitions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionAttribute.h"

namespace Seoul::Reflection
{

/**
 * @return The attribute id in this AttributeCollection, or nullptr
 * if id is not in this AttributeCollection.
 */
Attribute const* AttributeCollection::GetAttribute(HString id, Int iArg /*= -1*/) const
{
	for (auto i = m_vAttributes.Begin(); m_vAttributes.End() != i; ++i)
	{
		auto p = *i;
		if (p->GetArg() == iArg && id == p->GetId())
		{
			return p;
		}
	}

	return nullptr;
}

} // namespace Seoul::Reflection
