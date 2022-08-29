/**
 * \file Animation3DNetworkInstance.cpp
 * \brief 3D animation specialization of an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationIData.h"
#include "Animation3DClipDefinition.h"
#include "Animation3DDataInstance.h"
#include "Animation3DNetworkInstance.h"
#include "Animation3DPlayClipInstance.h"
#include "Animation3DState.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

NetworkInstance::~NetworkInstance()
{
}

NetworkInstance::NetworkInstance(
	const AnimationNetworkContentHandle& hNetwork,
	Animation::IData* pData,
	const SharedPtr<Animation::EventInterface>& pEventInterface,
	const InverseBindPoses& vInverseBindPoses)
	: Animation::NetworkInstance(hNetwork, pData, pEventInterface)
	, m_vInverseBindPoses(vInverseBindPoses)
{
}

void NetworkInstance::CommitSkinningPalette(
	RenderCommandStreamBuilder& rBuilder,
	const SharedPtr<Effect>& pEffect,
	HString parameterSemantic) const
{
	if (IsReady())
	{
		GetState().CommitSkinningPalette(rBuilder, pEffect, parameterSemantic);
	}
}

Animation::NodeInstance* NetworkInstance::CreatePlayClipInstance(
	const SharedPtr<Animation::PlayClipDefinition const>& pDef,
	const Animation::ClipSettings& settings)
{
	return SEOUL_NEW(MemoryBudgets::Animation3D) PlayClipInstance(*this, pDef, settings);
}

Animation::IState* NetworkInstance::CreateState()
{
	return SEOUL_NEW(MemoryBudgets::Animation3D) State(GetDataInterface(), GetEventInterface(), m_vInverseBindPoses);
}

NetworkInstance* NetworkInstance::CreateClone() const
{
	return SEOUL_NEW(MemoryBudgets::Animation3D) NetworkInstance(
		GetNetworkHandle(),
		GetDataInterface().Clone(),
		GetEventInterface(),
		m_vInverseBindPoses);
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
