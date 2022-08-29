/**
 * \file ContainerTestUtil.cpp
 * \brief Shared structures and functions for SeoulEngine
 * container testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContainerTestUtil.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "UnsafeBuffer.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

Int ContainerTestComplex::s_iCount(0);

SEOUL_SPEC_TEMPLATE_TYPE(List<ContainerTestComplex, 48>)
SEOUL_SPEC_TEMPLATE_TYPE(List<ContainerTestSimple, 48>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<ContainerTestComplex, 48>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<ContainerTestSimple, 48>)
SEOUL_BEGIN_TYPE(ContainerTestComplex)
	SEOUL_PROPERTY_N("FixedValue", m_iFixedValue)
	SEOUL_PROPERTY_N("VariableValue", m_iVariableValue)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(ContainerTestSimple)
	SEOUL_PROPERTY_N("A", m_iA)
	SEOUL_PROPERTY_N("B", m_B)
SEOUL_END_TYPE()

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
