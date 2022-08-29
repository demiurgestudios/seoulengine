/**
 * \file RenderSurface.cpp
 * \brief RenderSurface represents a set of GPU 2D render targets with
 * or without an attached depth buffer;
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDataStoreTableUtil.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderSurface.h"

namespace Seoul
{

/** Constants used to extract configuration values from a render surface configuration section. */
static const HString ksDepthStencil("DepthStencil");
static const HString ksRenderTarget("RenderTarget");

/**
 * Populate ruWidth and ruHeight with the dimensions of the active
 * render surface.
 *
 * \warning Calling this method from any thread other than the render thread
 * will trigger a SEOUL_FAIL().
 */
void RenderSurface2D::RenderThread_GetActiveSurfaceDimensions(Int32& riWidth, Int32& riHeight)
{
	CheckedPtr<RenderTarget> pTarget2D(RenderTarget::GetActiveRenderTarget());
	if (pTarget2D.IsValid())
	{
		riWidth = pTarget2D->GetWidth();
		riHeight = pTarget2D->GetHeight();
	}
	else
	{
		Viewport viewport = RenderDevice::Get()->GetBackBufferViewport();
		riWidth = viewport.m_iTargetWidth;
		riHeight = viewport.m_iTargetHeight;
	}
}

/**
 * Construct the RenderSurface2D from an JSON file section
 * describing its settings.
 */
RenderSurface2D::RenderSurface2D(const DataStoreTableUtil& configSettings)
	: m_pDepthStencilSurface(nullptr)
	, m_Name(configSettings.GetName())
{
	HString name;
	if (configSettings.GetValue<HString>(ksRenderTarget, name))
	{
		m_pRenderTarget.Reset(Renderer::Get()->GetRenderTarget(name));
	}
	else
	{
		m_pRenderTarget.Reset();
	}

	if (configSettings.GetValue<HString>(ksDepthStencil, name))
	{
		m_pDepthStencilSurface.Reset(Renderer::Get()->GetDepthStencilSurface(name));
	}
	else
	{
		m_pDepthStencilSurface.Reset();
	}
}

RenderSurface2D::~RenderSurface2D()
{
}

/**
 * Make this RenderSurface2D the active render suface.
 */
void RenderSurface2D::Select(RenderCommandStreamBuilder& rBuilder) const
{
	// Get the viewport of this RenderSurface2D.
	Viewport const viewport = GetViewport();

	// Select our render target.
	rBuilder.SelectRenderTarget(m_pRenderTarget.GetPtr());

	// Select our depth-stencil surface.
	rBuilder.SelectDepthStencilSurface(m_pDepthStencilSurface.GetPtr());

	// On some platforms, new surface settings do not become
	// active until the entire surface is committed.
	rBuilder.CommitRenderSurface();

	// Set the viewport for this RenderSurface's targets. This
	// must be done last - setting a DepthStencilSurface
	// can reset the viewport setting.
	rBuilder.SetCurrentViewport(viewport);
	rBuilder.SetScissor(true, ToClearSafeScissor(viewport));
}

/**
 * Resets all render targets and the depth-stencil surface.
 */
void RenderSurface2D::Reset(RenderCommandStreamBuilder& rBuilder)
{
	rBuilder.SelectRenderTarget(nullptr);
	rBuilder.SelectDepthStencilSurface(nullptr);

	CheckedPtr<RenderDevice> pDevice = RenderDevice::Get();

	// On some platforms, new surface settings do not become
	// active until the entire surface is committed.
	rBuilder.CommitRenderSurface();

	// Restore the back buffer viewport.
	Viewport const backBufferViewport = pDevice->GetBackBufferViewport();
	rBuilder.SetCurrentViewport(backBufferViewport);
	rBuilder.SetScissor(true, ToClearSafeScissor(backBufferViewport));
}

/**
 * @return The rendering viewport of this RenderSurface2D.
 */
Viewport RenderSurface2D::GetViewport() const
{
	// Set up the viewport for this RenderSurface. RenderSurfaces that
	// are not the BackBuffer always use a viewport that completely encloses
	// the entire surface.
	Viewport viewport = RenderDevice::Get()->GetBackBufferViewport();
	if (m_pRenderTarget.IsValid())
	{
		Int32 const iHeight = GetHeight();
		Int32 const iWidth = GetWidth();

		viewport.m_iTargetWidth = iWidth;
		viewport.m_iTargetHeight = iHeight;
		viewport.m_iViewportX = 0;
		viewport.m_iViewportY = 0;
		viewport.m_iViewportWidth = iWidth;
		viewport.m_iViewportHeight = iHeight;
	}

	return viewport;
}

/**
 * Returns the width of this render surface in pixels.
 */
Int32 RenderSurface2D::GetWidth() const
{
	Viewport const viewport = RenderDevice::Get()->GetBackBufferViewport();
	if (m_pRenderTarget.IsValid())
	{
		Int32 const iWidth = (m_pRenderTarget->IsWidthProportionalToBackBuffer()
			? Max(1, (Int32)(viewport.m_iViewportWidth * m_pRenderTarget->GetWidthProportion()))
			: m_pRenderTarget->GetWidth());
		return iWidth;
	}
	else
	{
		return viewport.m_iTargetWidth;
	}
}

/**
 * Returns the height of this render surface in pixels.
 */
Int32 RenderSurface2D::GetHeight() const
{
	Viewport const viewport = RenderDevice::Get()->GetBackBufferViewport();
	if (m_pRenderTarget.IsValid())
	{
		Int32 const iHeight = (m_pRenderTarget->IsHeightProportionalToBackBuffer()
			? Max(1, (Int32)(viewport.m_iViewportHeight * m_pRenderTarget->GetHeightProportion()))
			: m_pRenderTarget->GetHeight());
		return iHeight;
	}
	else
	{
		return viewport.m_iTargetHeight;
	}
}

/**
 * Returns true if the depth-stencil surface of this
 * RenderSurface2D has a stencil buffer, false otherwise.
 */
Bool RenderSurface2D::HasStencilBuffer() const
{
	if (m_pDepthStencilSurface.IsValid())
	{
		return m_pDepthStencilSurface->HasStencilBuffer();
	}
	else
	{
		return DepthStencilFormatHasStencilBuffer(RenderDevice::Get()->GetBackBufferDepthStencilFormat());
	}
}

} // namespace Seoul
