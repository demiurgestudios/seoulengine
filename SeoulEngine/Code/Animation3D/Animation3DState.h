/**
 * \file Animation3DState.h
 * \brief Binds runtime posable state into the common animation
 * framework.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_STATE_H
#define ANIMATION3D_STATE_H

#include "AnimationIState.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
namespace Seoul { namespace Animation { class IData; } }
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation3D { class DataInstance; } }

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class State SEOUL_SEALED : public Animation::IState
{
public:
	typedef Vector<Matrix3x4, MemoryBudgets::Rendering> InverseBindPoses;

	State(
		const Animation::IData& data,
		const SharedPtr<Animation::EventInterface>& pEventInterface,
		const InverseBindPoses& vInverseBindPoses);
	~State();

	const DataInstance& GetInstance() const { return *m_pInstance; }
	DataInstance& GetInstance() { return *m_pInstance; }
	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(State);

	ScopedPtr<DataInstance> m_pInstance;
}; // class State

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
