/**
 * \file AnimationNodeDefinition.cpp
 * \brief Base class of the read-only data of an animation
 * network node. Subclasses of this class are used to fully define
 * an animation network in content.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationNodeDefinition.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Animation::NodeDefinition>)
SEOUL_BEGIN_TYPE(Animation::NodeDefinition)
	SEOUL_ATTRIBUTE(PolymorphicKey, "$type")
SEOUL_END_TYPE()

namespace Animation
{

SEOUL_LINK_ME(class, BlendDefinition);
SEOUL_LINK_ME(class, PlayClipDefinition);
SEOUL_LINK_ME(class, StateMachineDefinition);

NodeDefinition::NodeDefinition()
{
}

NodeDefinition::~NodeDefinition()
{
}

} // namespace Animation

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
