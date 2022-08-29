/**
 * \file Animation3DNetworkInstance.h
 * \brief 2D animation specialization of an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_NETWORK_INSTANCE_H
#define ANIMATION3D_NETWORK_INSTANCE_H

#include "Animation3DData.h"
#include "Animation3DState.h"
#include "AnimationNetworkInstance.h"
namespace Seoul { class Effect; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { namespace Animation3D { class Manager; } }

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class NetworkInstance SEOUL_SEALED : public Animation::NetworkInstance
{
public:
	typedef Vector<Matrix3x4, MemoryBudgets::Rendering> InverseBindPoses;

	~NetworkInstance();

	// Instance data of this network. It is only safe to access these members when IsReady()
	// is true.
	const SharedPtr<DataDefinition const>& GetData() const
	{
		return static_cast<const Data&>(GetDataInterface()).GetPtr();
	}
	const Animation3DDataContentHandle& GetDataHandle() const
	{
		return static_cast<const Data&>(GetDataInterface()).GetHandle();
	}

	const DataInstance& GetState() const
	{
		return static_cast<const State&>(GetStateInterface()).GetInstance();
	}
	DataInstance& GetState()
	{
		return static_cast<State&>(GetStateInterface()).GetInstance();
	}

	void CommitSkinningPalette(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const;

	virtual Animation::NodeInstance* CreatePlayClipInstance(
		const SharedPtr<Animation::PlayClipDefinition const>& pDef,
		const Animation::ClipSettings& settings) SEOUL_OVERRIDE;

private:
	friend class Manager;

	InverseBindPoses m_vInverseBindPoses;

	virtual Animation::IState* CreateState() SEOUL_OVERRIDE;
	virtual NetworkInstance* CreateClone() const SEOUL_OVERRIDE;

	NetworkInstance(
		const AnimationNetworkContentHandle& hNetwork,
		Animation::IData* pData,
		const SharedPtr<Animation::EventInterface>& pEventInterface,
		const InverseBindPoses& vInverseBindPoses);

	SEOUL_DISABLE_COPY(NetworkInstance);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(NetworkInstance);
}; // class NetworkInstance

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
