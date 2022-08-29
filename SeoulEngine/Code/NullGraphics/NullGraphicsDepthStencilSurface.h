/**
 * \file NullGraphicsDepthStencilSurface.h
 * \brief Nop implementation of a DepthStencilSurface for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_DEPTH_STENCIL_SURFACE_H
#define NULL_GRAPHICS_DEPTH_STENCIL_SURFACE_H

#include "DepthStencilSurface.h"
#include "UnsafeHandle.h"

namespace Seoul
{

class NullGraphicsDepthStencilSurface SEOUL_SEALED : public DepthStencilSurface
{
public:
	// BaseGraphicsObject overrides
	virtual void OnLost() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;
	// /BaseGraphicsObject overrides

	// BaseTexture overrides
	virtual UnsafeHandle GetTextureHandle() const SEOUL_OVERRIDE
	{
		// Resolve depth-stencil surfaces to texture is not
		// supported in NullGraphics.
		return UnsafeHandle();
	}
	// /BaseTexture overrides

	virtual void Select() SEOUL_OVERRIDE;
	virtual void Unselect() SEOUL_OVERRIDE;
	virtual void Resolve() SEOUL_OVERRIDE;

private:
	NullGraphicsDepthStencilSurface(const DataStoreTableUtil& configSettings);
	~NullGraphicsDepthStencilSurface();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsDepthStencilSurface);

	friend class NullGraphicsDevice;
	friend class DepthStencilSurface;

	SEOUL_DISABLE_COPY(NullGraphicsDepthStencilSurface);
}; // class NullGraphicsDepthStencilSurface

} // namespace Seoul

#endif // include guard
