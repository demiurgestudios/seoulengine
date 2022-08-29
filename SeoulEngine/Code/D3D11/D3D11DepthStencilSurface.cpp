/**
 * \file D3D11DepthStencilSurface.cpp
 * \brief Specialization of D3D11DepthStencilSurface for D3D11 rendering on PC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3D11DepthStencilSurface.h"
#include "D3D11Device.h"
#include "ThreadId.h"

namespace Seoul
{

D3D11DepthStencilSurface::D3D11DepthStencilSurface(const DataStoreTableUtil& configSettings)
	: DepthStencilSurface(configSettings)
	, m_pTexture(nullptr)
	, m_pDepthStencilView(nullptr)
{
}

D3D11DepthStencilSurface::~D3D11DepthStencilSurface()
{
	SEOUL_ASSERT(!m_pDepthStencilView.IsValid());
	SEOUL_ASSERT(!m_pTexture.IsValid());
}

/**
 * Reselects the depth-stencil surface currently marked as selected.
 * This method can be used to restore the selected depth-stencil surface
 * when something underneath or oustide Seoul engine has changed the
 * depth-stencil surface.
 */
void D3D11DepthStencilSurface::Reselect()
{
	SEOUL_ASSERT(IsRenderThread());

	GetD3D11Device().SetDepthStencilSurface(this);
}

/**
 * Make this D3D11DepthStencilSurface the active D3D11DepthStencilSurface.
 */
void D3D11DepthStencilSurface::Select()
{
	SEOUL_ASSERT(IsRenderThread());

	if (this != s_pCurrentSurface)
	{
		GetD3D11Device().SetDepthStencilSurface(this);

		s_pCurrentSurface = this;
	}
}

/**
 * Unset all D3D11DepthStencilSurfaces from the device.
 */
void D3D11DepthStencilSurface::Unselect()
{
	SEOUL_ASSERT(IsRenderThread());
	if (this == s_pCurrentSurface)
	{
		GetD3D11Device().SetDepthStencilSurface(nullptr);
		s_pCurrentSurface = nullptr;
	}
}

/**
 * Resolves this D3D11DepthStencilSurface to its texture.
 *
 * This is a nop on PC. D3D11DepthStencilSurfaces cannot be resolved on PC.
 */
void D3D11DepthStencilSurface::Resolve()
{
	// Nop
}

/**
 * When a D3D11DepthStencilSurface is lost, all its GPU resources
 * are gone, so CPU resources must be cleaned up as well.
 */
void D3D11DepthStencilSurface::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	Unselect();

	DepthStencilSurface::OnLost();

	SafeRelease(m_pDepthStencilView);
	SafeRelease(m_pTexture);
}

/**
 * When the graphics device has reset, the D3D11DepthStencilSurface
 *  also needs to recreate its resources.
 */
void D3D11DepthStencilSurface::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	D3D11Device& rDevice = GetD3D11Device();
	SEOUL_ASSERT(rDevice.GetD3DDevice());

	// Refresh the width and height, in case this DepthStencilSurface
	// is defined relative to the back buffer width and height.
	InternalRefreshWidthAndHeight();

	D3D11_TEXTURE2D_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DepthStencilFormatToD3D(m_eFormat);
	textureDesc.Height = GetHeight();
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.Width = GetWidth();
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	memset(&depthStencilViewDesc, 0, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DepthStencilFormatToD3D(m_eFormat);
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	ID3D11Texture2D* pTexture = nullptr;
	ID3D11DepthStencilView* pDepthStencilView = nullptr;
	if (FAILED(rDevice.GetD3DDevice()->CreateTexture2D(
		&textureDesc,
		nullptr,
		&pTexture)))
	{
		goto error;
	}

	if (FAILED(rDevice.GetD3DDevice()->CreateDepthStencilView(
		pTexture,
		&depthStencilViewDesc,
		&pDepthStencilView)))
	{
		goto error;
	}
	m_pDepthStencilView = pDepthStencilView;
	m_pTexture = pTexture;

	DepthStencilSurface::OnReset();

	return;

error:
	SafeRelease(pDepthStencilView);
	SafeRelease(pTexture);
}

} // namespace Seoul
