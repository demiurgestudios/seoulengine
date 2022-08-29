/**
 * \file Animation3DState.cpp
 * \brief Binds runtime posable state into the common animation
 * framework.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Animation3DClipDefinition.h"
#include "Animation3DData.h"
#include "Animation3DDataInstance.h"
#include "Animation3DState.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

State::State(
	const Animation::IData& data,
	const SharedPtr<Animation::EventInterface>& pEventInterface,
	const InverseBindPoses& vInverseBindPoses)
	: m_pInstance(SEOUL_NEW(MemoryBudgets::Animation3D) DataInstance(static_cast<const Data&>(data).GetPtr(), pEventInterface, vInverseBindPoses))
{
}

State::~State()
{
}

void State::Tick(Float fDeltaTimeInSeconds)
{
	// Apply the animation cache prior to pose.
	m_pInstance->ApplyCache();

	// TODO: Break this out into a separate Pose(), so it
	// is only called if actually rendering a frame.
	m_pInstance->PoseSkinningPalette();
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
