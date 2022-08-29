/**
 * \file AnimationBlendDefinition.cpp
 * \brief Defines a blend node in an animation graph. This is read-only
 * data at runtime. To evaluate a blend node, you must instantiate
 * an Animation::BlendInstance, which will normally occur as part
 * of creating an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationBlendDefinition.h"
#include "AnimationBlendInstance.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_BEGIN_TYPE(Animation::BlendDefinition, TypeFlags::kDisableCopy)
	SEOUL_TYPE_ALIAS("AnimBlend")
	SEOUL_PARENT(Animation::NodeDefinition)
	SEOUL_PROPERTY_N("ChildA", m_pChildA)
	SEOUL_PROPERTY_N("ChildB", m_pChildB)
	SEOUL_PROPERTY_N("Mix", m_MixParameterId)
	SEOUL_PROPERTY_N("SynchronizeTime", m_bSynchronizeTime)
SEOUL_END_TYPE()

namespace Animation
{

BlendDefinition::BlendDefinition()
	: m_pChildA()
	, m_pChildB()
	, m_MixParameterId()
	, m_bSynchronizeTime(false)
{
}

BlendDefinition::~BlendDefinition()
{
}

NodeInstance* BlendDefinition::CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const
{
	return SEOUL_NEW(MemoryBudgets::Animation) BlendInstance(r, SharedPtr<BlendDefinition const>(this), creationData);
}

} // namespace Animation

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
