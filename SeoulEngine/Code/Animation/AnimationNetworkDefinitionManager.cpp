/**
 * \file AnimationNetworkDefinitionManager.cpp
 * \brief Global singleton that manages animation and network data in
 * the SeoulEngine content system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationNetworkDefinitionManager.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

NetworkDefinitionManager::NetworkDefinitionManager()
	: m_NetworkContent()
{
}

NetworkDefinitionManager::~NetworkDefinitionManager()
{
}

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
