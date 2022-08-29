/**
 * \file NullGraphicsRenderTarget.h
 * \brief Nop implementation of a RenderTarget for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_RENDER_TARGET_2D_H
#define NULL_GRAPHICS_RENDER_TARGET_2D_H

#include "RenderTarget.h"
#include "UnsafeHandle.h"

namespace Seoul
{

class NullGraphicsRenderTarget SEOUL_SEALED : public RenderTarget
{
public:
	// BaseGraphicsObject overrides
	virtual void OnLost() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;
	// /BaseGraphicsObject overrides

	// BaseTexture overrides
	virtual UnsafeHandle GetTextureHandle() const SEOUL_OVERRIDE
	{
		return UnsafeHandle((NullGraphicsRenderTarget*)this);
	}
	// /BaseTexture overrides

	virtual void Select() SEOUL_OVERRIDE;
	virtual void Unselect() SEOUL_OVERRIDE;
	virtual void Resolve() SEOUL_OVERRIDE;

private:
	NullGraphicsRenderTarget(const DataStoreTableUtil& configSettings);
	~NullGraphicsRenderTarget();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsRenderTarget);

	friend class NullGraphicsDevice;

private:
	SEOUL_DISABLE_COPY(NullGraphicsRenderTarget);
}; // class NullGraphicsRenderTarget

} // namespace Seoul

#endif // include guard
