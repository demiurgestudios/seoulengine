/**
 * \file AnimationNodeInstance.cpp
 * \brief Base class of the runtime instantiation of an animation
 * network node. Subclasses of this class are used to perform runtime
 * process of an animation network.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationNodeInstance.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

NodeInstance::NodeInstance()
{
}

NodeInstance::~NodeInstance()
{
}

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
