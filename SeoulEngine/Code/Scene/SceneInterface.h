/**
 * \file SceneInterface.h
 * \brief Interface for a scene, provides scene-wide access.
 *
 * Different contexts can implement different forms of scene management.
 * Interface provides a full abstract, base interface for Component
 * and Object to access common, scene-wide functionality (e.g.
 * GetObjectById()).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_INTERFACE_H
#define SCENE_INTERFACE_H

#include "SharedPtr.h"
#include "SeoulString.h"
namespace Seoul { struct Vector3D; }
namespace Seoul { namespace Physics { class Simulator; } }
namespace Seoul { namespace Reflection { class Type; } }
namespace Seoul { namespace Scene { class Component; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class Interface SEOUL_ABSTRACT
{
public:
	typedef Vector<SharedPtr<Object>, MemoryBudgets::SceneObject> Objects;

	Interface()
	{
	}

	virtual ~Interface()
	{
	}

	// Optional implementation.

	/** When defined, equivalent to Camera::ConvertScreenSpaceToWorldSpace() for the scene. */
	virtual Bool ConvertScreenSpaceToWorldSpace(
		const Vector3D& vScreenSpace,
		Vector3D& rvOut) const
	{
		return false;
	}

	// Required implementation.
	virtual const Objects& GetObjects() const = 0;
	virtual Bool GetObjectById(const String& sId, SharedPtr<Object>& rpObject) const = 0;
	virtual Physics::Simulator* GetPhysicsSimulator() const = 0;
	// /Required implementation.

private:
	SEOUL_DISABLE_COPY(Interface);
}; // class Interface

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
