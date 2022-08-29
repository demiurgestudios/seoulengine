/**
 * \file SceneSetTransformComponent.h
 * \brief Subclass of components that provide both read and write access to transform data.
 *
 * SetTransformComponent is inherited by any Component that provides
 * read-write access to *Rotation() and *Position(), defining a mutable
 * spatial location of the owner Object in 3D space.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_SET_TRANSFORM_COMPONENT_H
#define SCENE_SET_TRANSFORM_COMPONENT_H

#if SEOUL_WITH_SCENE

#include "Matrix4D.h"
#include "SceneGetTransformComponent.h"

namespace Seoul::Scene
{

class SetTransformComponent SEOUL_ABSTRACT : public GetTransformComponent
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SetTransformComponent);

	virtual ~SetTransformComponent()
	{
	}

	virtual Bool CanSetTransform() const SEOUL_OVERRIDE { return true; }

	virtual void SetRotation(const Quaternion& qRotation) = 0;
	virtual void SetPosition(const Vector3D& vPosition) = 0;

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(SetTransformComponent);

	SetTransformComponent()
	{
	}

private:
	SEOUL_DISABLE_COPY(SetTransformComponent);
}; // class SetTransformComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
