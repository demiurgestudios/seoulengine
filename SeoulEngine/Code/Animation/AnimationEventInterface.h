/**
 * \file AnimationEventInterface.h
 * \brief Interface that can be implemented to listen for and handle
 * animation callbacks.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_EVENT_INTERFACE_H
#define ANIMATION_EVENT_INTERFACE_H

#include "SeoulHString.h"
#include "SharedPtr.h"
namespace Seoul { class String; }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class EventInterface SEOUL_ABSTRACT
{
public:
	virtual ~EventInterface()
	{
	}

	virtual void DispatchEvent(HString name, Int32 i, Float32 f, const String& s) = 0;

	// Optional override - called once per frame by a NetworkInstance::Tick(),
	// after all animation processing has completed for that frame.
	virtual void Tick(Float fDeltaTimeInSeconds)
	{
		// Nop
	}

protected:
	SEOUL_REFERENCE_COUNTED(EventInterface);

	EventInterface()
	{
	}

private:
	SEOUL_DISABLE_COPY(EventInterface);
}; // class EventInterface

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
