/**
 * \file RenderSurface.h
 * \brief RenderSurface represents a set of GPU 2D render targets with
 * or without an attached depth buffer;
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef RENDER_SURFACE_H
#define RENDER_SURFACE_H

#include "BaseGraphicsObject.h"
#include "DepthStencilSurface.h"
#include "SharedPtr.h"
#include "RenderTarget.h"
#include "Texture.h"
#include "Viewport.h"
namespace Seoul { class DataStoreTableUtil; }
namespace Seoul { class RenderCommandStreamBuilder; }

namespace Seoul
{

/**
 * GPUs can render their output to multiple framebuffers
 * simultaneously. A RenderSurface is a collection of render targets
 * and a depth-stencil buffer that is used for rendering.
 */
class RenderSurface2D SEOUL_SEALED
{
public:
	static const Int32 kInvalidSubSurfaceHandle = -1;

	// Fill ruWidth and ruHeight with the current dimensions of the active
	// render surface - this function can only be called from the render
	// thread.
	static void RenderThread_GetActiveSurfaceDimensions(Int32& riWidth, Int32& riHeight);

	/**
	 * Returns true if iSubSurfaceHandle is a valid surface
	 * handle, false otherwise.
	 */
	static Bool IsValidSubSurfaceHandle(Int32 iSubSurfaceHandle)
	{
		return (iSubSurfaceHandle >= 0);
	}

	RenderSurface2D(const DataStoreTableUtil& configSettings);
	~RenderSurface2D();

	void Select(RenderCommandStreamBuilder& rBuilder) const;
	static void Reset(RenderCommandStreamBuilder& rBuilder);

	/**
	 * Returns the name of this RenderSurface2D.
	 *
	 * The name is the same as the section name of the JSON file
	 * used to configure this RenderSurface2D.
	 */
	HString GetName() const
	{
		return m_Name;
	}

	Viewport GetViewport() const;
	Int32 GetHeight() const;
	Int32 GetWidth() const;

	Int32 GetSubSurfaceWidth() const;
	Int32 GetSubSurfaceHeight() const;

	/**
	 * Returns the DepthStencil surface of this RenderSurface.
	 *
	 * The returned value can be nullptr. If nullptr, it indicates that this
	 * RenderSurface is using the auto-generated depth-stencil surface.
	 */
	DepthStencilSurface* GetDepthStencilSurface() const
	{
		return m_pDepthStencilSurface.GetPtr();
	}

	/**
	 * Returns the RenderTarget of this RenderSurface.
	 *
	 * The returned value can be nullptr. If nullptr, it indicates that this
	 * RenderSurface is using the back buffer.
	 */
	RenderTarget* GetRenderTarget() const
	{
		return m_pRenderTarget.GetPtr();
	}

	Bool HasStencilBuffer() const;

private:
	SharedPtr<DepthStencilSurface> m_pDepthStencilSurface;
	SharedPtr<RenderTarget> m_pRenderTarget;

	HString m_Name;
}; // class RenderSurface2D

} // namespace Seoul

#endif // include guard
