/**
 * \file FxStudioComponentBase.h
 * \brief Base class for all SeoulEngine types that will implement
 * the FxStudio::Component interface.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_COMPONENT_BASE_H
#define FX_STUDIO_COMPONENT_BASE_H

#include "Matrix4D.h"
#include "Prereqs.h"
#include "Vector3D.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul::FxStudio
{

/**
 * Common base class that must be subclassed by any SeoulEngine
 * type that implements the ::FxStudio::Component interface.
 */
class ComponentBase SEOUL_ABSTRACT : public ::FxStudio::Component
{
public:
	ComponentBase(const InternalDataType& internalData);
	virtual ~ComponentBase();

	// Common method for all components, should return
	// true if the Component requires Render() calls.
	virtual Bool NeedsRender() const
	{
		// false by default.
		return false;
	}

	// Common method for all components, updates configuration
	// flags of the component.
	virtual void SetFlags(UInt32 uFlags)
	{
		// Nop by default.
	}

	// Common method for all components, updates the world position
	// of the component.
	virtual void SetPosition(const Vector3D& vPosition)
	{
		// Nop by default.
	}

	// Common method for all components, sets the current gravity
	virtual void SetGravity(Float fGravityAcceleration)
	{
		// Nop by default.
	}

	// Common method for all components, updates the world transform
	// of the component.
	virtual void SetTransform(const Matrix4D& mTransform)
	{
		// Nop by default.
	}

	// Common method for all components, updates the parent
	// transform to use if in world space
	virtual void SetParentIfWorldspace(const Matrix4D& mTransform)
	{
		// Nop by default.
	}

	// Gets the string representation of the type if this component
	virtual HString GetComponentTypeName() const = 0;

	// Common method for all components, get the rally point
	// of the component.
	virtual Bool GetParticleRallyPointState(Vector3D& rvPoint, Bool& rbUseRallyPoint) const
	{
		// Nop by default.
		return false;
	}

	// Common method for all components, updates the rally point
	// of the component.
	virtual void SetParticleRallyPointOverride(const Vector3D& vRallyPoint)
	{
		// Nop by default.
	}

private:
	SEOUL_DISABLE_COPY(ComponentBase);
}; // class FxStudio::ComponentBase

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
