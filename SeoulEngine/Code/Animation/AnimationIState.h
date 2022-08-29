/**
 * \file AnimationIState.h
 * \brief Abstracted interface to animation state.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_ISTATE_H
#define ANIMATION_ISTATE_H

#include "Prereqs.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class IState SEOUL_ABSTRACT
{
public:
	virtual ~IState()
	{
	}

	virtual void Tick(Float fDeltaTimeInSeconds) = 0;

protected:
	IState()
	{
	}

private:
	SEOUL_DISABLE_COPY(IState);
}; // class IState

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
