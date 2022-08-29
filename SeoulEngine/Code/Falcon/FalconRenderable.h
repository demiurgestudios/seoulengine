/**
 * \file FalconRenderable.h
 * \brief An interface for any instance that will
 * be rendered by the Falcon render backend.
 *
 * Most renderable things in Falcon are expected to inherit
 * from Falcon::Instance (they are nodes in a Falcon scene
 * graph). For special cases, non-nodes can implement
 * Falcon::Renderable for render preparation by Falcon::Render::Poser
 * and rend submission by Falcon::Render::Drawer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDERABLE_H
#define FALCON_RENDERABLE_H

#include "SharedPtr.h"
#include "Vector2D.h"
namespace Seoul { namespace Falcon { struct ColorTransformWithAlpha; } }
namespace Seoul { namespace Falcon { namespace Render { class Drawer; } } }
namespace Seoul { struct Matrix2x3; }
namespace Seoul { namespace Falcon { struct TextureReference; } }
namespace Seoul { namespace Falcon { struct Rectangle; } }

namespace Seoul::Falcon
{

class Renderable SEOUL_ABSTRACT
{
public:
	virtual ~Renderable()
	{
	}

	virtual void Draw(
		Render::Drawer& /*rDrawer*/,
		const Rectangle& /*worldBoundsPreClip*/,
		const Matrix2x3& /*mWorld*/,
		const ColorTransformWithAlpha& /*cxWorld*/,
		const TextureReference& /*textureReference*/,
		Int32 /*iSubInstanceId*/) = 0;

	virtual Bool CastShadow() const = 0;
	virtual Vector2D GetShadowPlaneWorldPosition() const = 0;

protected:
	SEOUL_REFERENCE_COUNTED(Renderable);

	Renderable()
	{
	}

private:
	SEOUL_DISABLE_COPY(Renderable);
}; // class Renderable

} // namespace Seoul::Falcon

#endif // include guard
