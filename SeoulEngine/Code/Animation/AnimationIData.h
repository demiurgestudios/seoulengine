/**
 * \file AnimationIData.h
 * \brief Abstracted interface to animation state.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_IDATA_H
#define ANIMATION_IDATA_H

#include "Prereqs.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class IData SEOUL_ABSTRACT
{
public:
	virtual ~IData()
	{
	}

	virtual void AcquireInstance() = 0;
	virtual IData* Clone() const = 0;
	virtual Atomic32Type GetTotalLoadsCount() const = 0;
	virtual Bool HasInstance() const = 0;
	virtual Bool IsLoading() const = 0;
	virtual void ReleaseInstance() = 0;

protected:
	IData()
	{
	}

private:
	SEOUL_DISABLE_COPY(IData);
}; // class IData

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
