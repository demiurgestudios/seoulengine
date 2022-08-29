/**
 * \file OGLES2DepthStencilSurface.h
 * \brief Specialization of DepthStencilSurface for OGLES2.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_DEPTH_STENCIL_SURFACE_H
#define OGLES2_DEPTH_STENCIL_SURFACE_H

#include "DepthStencilSurface.h"
#include "OGLES2Util.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/**
 * OGLES2DepthStencilSurface encapsulates a depth-stencil surface
 * in OpenGL.
 */
class OGLES2DepthStencilSurface SEOUL_SEALED : public DepthStencilSurface
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
		// Nop - cannot use depth-stencil surfaces for input under
		// OpenGL ES2.
		return UnsafeHandle();
	}
	// /BaseTexture overrides

	// DepthStencilSurface overrides
	virtual void Select() SEOUL_OVERRIDE;
	virtual void Unselect() SEOUL_OVERRIDE;
	virtual void Resolve() SEOUL_OVERRIDE;
	// /DepthStencilSurface overrides

private:
	OGLES2DepthStencilSurface(const DataStoreTableUtil& configSettings);
	~OGLES2DepthStencilSurface();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2DepthStencilSurface);

	Bool InternalCreate();
	void InternalDestroy();

	friend class OGLES2RenderDevice;
	friend class DepthStencilSurface;
	GLuint m_DepthSurface;
	GLuint m_StencilSurface;

	SEOUL_DISABLE_COPY(OGLES2DepthStencilSurface);
}; // class OGLES2DepthStencilSurface

} // namespace Seoul

#endif // include guard
