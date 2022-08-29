/**
 * \file AnimationNodeType.cpp
 * \brief All node types in an animation graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationNodeType.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_BEGIN_ENUM(Animation::NodeType)
	SEOUL_ENUM_N("Blend", Animation::NodeType::kBlend)
	SEOUL_ENUM_N("PlayClip", Animation::NodeType::kPlayClip)
	SEOUL_ENUM_N("StateMachine", Animation::NodeType::kStateMachine)
SEOUL_END_ENUM()

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
