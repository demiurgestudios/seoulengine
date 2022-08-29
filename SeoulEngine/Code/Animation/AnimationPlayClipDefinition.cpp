/**
 * \file AnimationPlayClipDefinition.cpp
 * \brief Defines a clip playback node in an animation graph. This is read-only
 * data at runtime. To evaluate a play clip node, you must instantiate
 * an Animation::PlayClipInstance, which will normally occur as part
 * of creating an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationNetworkInstance.h"
#include "AnimationPlayClipDefinition.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_BEGIN_TYPE(Animation::PlayClipDefinition, TypeFlags::kDisableCopy)
	SEOUL_TYPE_ALIAS("AnimPlayClip")
	SEOUL_PARENT(Animation::NodeDefinition)
	SEOUL_PROPERTY_N("OnComplete", m_OnComplete)
	SEOUL_PROPERTY_N("Name", m_Name)
	SEOUL_PROPERTY_N("Loop", m_bLoop)
	SEOUL_PROPERTY_N("EventMixThreshold", m_fEventMixThreshold)
SEOUL_END_TYPE()

namespace Animation
{

PlayClipDefinition::PlayClipDefinition()
	: m_Name()
	, m_OnComplete()
	, m_fEventMixThreshold(0.0f)
	, m_bLoop(false)
{
}

PlayClipDefinition::~PlayClipDefinition()
{
}

NodeInstance* PlayClipDefinition::CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const
{
	ClipSettings settings;
	settings.m_fEventMixThreshold = m_fEventMixThreshold;
	return r.CreatePlayClipInstance(SharedPtr<PlayClipDefinition const>(this), settings);
}

} // namespace Animation

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
