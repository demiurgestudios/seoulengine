/**
 * \file ReflectionMethod.cpp
 * \brief Reflection object used to define a reflectable method of a reflectable
 * class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionAttribute.h"
#include "ReflectionMethod.h"

namespace Seoul::Reflection
{

const MethodArguments Method::k0Arguments;

Method::Method(HString name)
	: m_Name(name)
{
}

Method::~Method()
{
	m_Attributes.DestroyAttributes();
}

} // namespace Seoul::Reflection
