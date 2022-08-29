/**
 * \file OGLES2RenderTarget.h
 * \brief Specialization of RenderTarget for the OGLES2 platform. RenderTarget
 * encapsulates a renderable color surface.
 *
 * In OGLES2, RenderTarget can potentially encapsulate
 * 2 renderable color surfaces, to allow a surface to be sampled while it
 * is the render target.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_RENDER_TARGET_2D_H
#define OGLES2_RENDER_TARGET_2D_H

#include "RenderTarget.h"
#include "OGLES2RenderTarget.h"
#include "OGLES2Util.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/**
 * OGLES2RenderTarget encapsulates a renderable color buffer on OGLES2,
 * that can also be sampled as a texture.
 */
class OGLES2RenderTarget SEOUL_SEALED : public RenderTarget
{
public:
	// BaseGraphicsObject overrides
	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;
	// /BaseGraphicsObject overrides

	// BaseTexture overrides
	virtual UnsafeHandle GetTextureHandle() const SEOUL_OVERRIDE
	{
		return UnsafeHandle(m_TextureB);
	}
	// /BaseTexture overrides

	// RenderTarget overrides
	virtual void Select() SEOUL_OVERRIDE;
	virtual void Unselect() SEOUL_OVERRIDE;
	virtual void Resolve() SEOUL_OVERRIDE;
	// /RenderTarget overrides

private:
	OGLES2RenderTarget(const DataStoreTableUtil& configSettings);
	virtual ~OGLES2RenderTarget();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2RenderTarget);

	Bool InternalCreate();
	void InternalDestroy();

	friend class OGLES2RenderDevice;
	friend class RenderTarget;
	GLuint m_TextureA;
	GLuint m_TextureB;

	SEOUL_DISABLE_COPY(OGLES2RenderTarget);
}; // class OGLES2RenderTarget

} // namespace Seoul

#endif // include guard
