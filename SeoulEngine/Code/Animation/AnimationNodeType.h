/**
 * \file AnimationNodeType.h
 * \brief All node types in an animation graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_NODE_TYPE_H
#define ANIMATION_NODE_TYPE_H

#include "Prereqs.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

enum class NodeType
{
	/** BlendDefinition nodes combine 2 child nodes based on a blending parameter. */
	kBlend,

	/** Play clip nodes run a single animation clip. */
	kPlayClip,

	/** State machine nodes define a graph of states and transitions to handle various animated entity setups. */
	kStateMachine,
};

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
