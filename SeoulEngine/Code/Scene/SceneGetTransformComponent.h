/**
 * \file SceneGetTransformComponent.h
 * \brief Subclass of components that provide at least read access to transform data.
 *
 * GetTransformComponent is inherited by any Component that provides
 * at least read access to GetRotation() and GetPosition(), defining the
 * spatial location of the owner Object in 3D space.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_GET_TRANSFORM_COMPONENT_H
#define SCENE_GET_TRANSFORM_COMPONENT_H

#if SEOUL_WITH_SCENE

#include "Matrix4D.h"
#include "SceneComponent.h"

namespace Seoul::Scene
{

class GetTransformComponent SEOUL_ABSTRACT : public Component
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SceneTransformComponent);

	virtual ~GetTransformComponent()
	{
	}

	virtual Bool CanGetTransform() const SEOUL_OVERRIDE { return true; }

	virtual Quaternion GetRotation() const = 0;
	virtual Vector3D GetPosition() const = 0;

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(GetTransformComponent);

	GetTransformComponent()
	{
	}

private:
	SEOUL_DISABLE_COPY(GetTransformComponent);
}; // class SceneTransformComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
