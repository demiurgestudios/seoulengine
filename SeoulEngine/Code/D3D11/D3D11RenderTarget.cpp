/**
 * \file D3D11RenderTarget.cpp
 * \brief Specialization of RenderTarget for D3D11 on the PC platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3D11Device.h"
#include "D3D11RenderTarget.h"
#include "ThreadId.h"

namespace Seoul
{

/**
 * Construct this D3D11RenderTarget from a DataStore
 * that describes it. If an error occurs, D3D11RenderTarget will be left
 * in its default (invalid) state and will not be usable as a render target.
 */
D3D11RenderTarget::D3D11RenderTarget(const DataStoreTableUtil& configSettings)
	: RenderTarget(configSettings)
	, m_pTextureA(nullptr)
	, m_pTextureB(nullptr)
	, m_pShaderResourceViewA(nullptr)
	, m_pShaderResourceViewB(nullptr)
	, m_pRenderTargetViewA(nullptr)
	, m_pRenderTargetViewB(nullptr)
{
}

D3D11RenderTarget::~D3D11RenderTarget()
{
	SEOUL_ASSERT(IsRenderThread());

	// Unless a target needs to change with changes to
	// the back buffer, its resources are not destroyed
	// until the object is. Do so now.
	if (!IsProportional())
	{
		InternalDestroy();
	}
}

/**
 * Reselects this render target, can be used if
 * device state becomes inconsistent with SeoulEngine state.
 */
void D3D11RenderTarget::Reselect()
{
	SEOUL_ASSERT(IsRenderThread());

	if (s_pActiveRenderTarget == this)
	{
		GetD3D11Device().SetRenderTarget(this);
	}
}

/**
 * Called by the device when this D3D11RenderTarget needs to
 * be set as the active render target.
 */
void D3D11RenderTarget::Select()
{
	SEOUL_ASSERT(IsRenderThread());

	// Avoid redundant sets of the same render target, however, if this
	// is an input-output target, we must re-select the target, since the
	// actual surface may have changed.
	if (SupportsSimultaneousInputOutput() || s_pActiveRenderTarget != this)
	{
		auto& rd = GetD3D11Device();
		rd.SetRenderTarget(this);
		s_pActiveRenderTarget = this;
	}
}

/**
 * Resolves this RenderTarget to its texture.
 *
 * In D3D11, this simply swaps the active surface and texture.
 */
void D3D11RenderTarget::Resolve()
{
	SEOUL_ASSERT(IsRenderThread());
	if (SupportsSimultaneousInputOutput())
	{
		Swap(m_pRenderTargetViewA, m_pRenderTargetViewB);
		Swap(m_pShaderResourceViewA, m_pShaderResourceViewB);
		Swap(m_pTextureA, m_pTextureB);
	}
}

/**
 * Reset the render target to its default state,
 * if this render target is the currently active target.
 */
void D3D11RenderTarget::Unselect()
{
	SEOUL_ASSERT(IsRenderThread());

	if (s_pActiveRenderTarget != this)
	{
		return;
	}

	// Reset.
	auto& rd = GetD3D11Device();
	rd.SetRenderTarget(nullptr);

	// Clear.
	s_pActiveRenderTarget = nullptr;
}

/**
 * On create, if not a proportional target, create
 * resources.
 */
Bool D3D11RenderTarget::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Unless a render target's resources are proportional
	// to the back buffer, we create its resources in OnCreate().
	if (!IsProportional())
	{
		// Creation failure is an OnCreate() failure.
		if (!InternalCreate())
		{
			return false;
		}
	}

	SEOUL_VERIFY(RenderTarget::OnCreate());
	return true;
}

/**
 * On lost, resets all memory pointers and places this
 * OGLES2RenderTarget into the lost state.
 */
void D3D11RenderTarget::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	Unselect();

	RenderTarget::OnLost();

	// We must cleanup proportional resources in OnLost().
	if (IsProportional())
	{
		InternalDestroy();
	}
}

/**
 * Actually initializes the renderable state of this
 * RenderTarget.
 */
void D3D11RenderTarget::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	// Proportional resources are created
	// in OnReset() and destroyed in OnLost(). Otherwise,
	// they remain alive for the life of the object
	// under OGLES2.
	Bool bReset = true;
	if (IsProportional())
	{
		bReset = InternalCreate();
	}

	// Success, call the parent implementation.
	if (bReset)
	{
		RenderTarget::OnReset();
	}
}

/** Shared creation of target resources. */
Bool D3D11RenderTarget::InternalCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Sanity check
	SEOUL_ASSERT(!m_pRenderTargetViewA.IsValid());
	SEOUL_ASSERT(!m_pRenderTargetViewB.IsValid());
	SEOUL_ASSERT(!m_pShaderResourceViewA.IsValid());
	SEOUL_ASSERT(!m_pShaderResourceViewB.IsValid());
	SEOUL_ASSERT(!m_pTextureA.IsValid());
	SEOUL_ASSERT(!m_pTextureB.IsValid());

	auto& rDevice = GetD3D11Device();
	SEOUL_ASSERT(rDevice.GetD3DDevice());

	// Refresh the width and height, in case they are dependent on
	// the back buffer.
	InternalRefreshWidthAndHeight();

	D3D11_TEXTURE2D_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = PixelFormatToD3D(m_eFormat);
	textureDesc.Height = GetHeight();
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.Width = GetWidth();

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	memset(&shaderResourceViewDesc, 0, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = PixelFormatToD3D(m_eFormat);
	shaderResourceViewDesc.Texture2D.MipLevels = 1u;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0u;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc;
	memset(&renderTargetDesc, 0, sizeof(renderTargetDesc));
	renderTargetDesc.Format = PixelFormatToD3D(m_eFormat);
	renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	ID3D11Texture2D* pTextureA = nullptr;
	ID3D11Texture2D* pTextureB = nullptr;
	ID3D11RenderTargetView* pRenderTargetViewA = nullptr;
	ID3D11RenderTargetView* pRenderTargetViewB = nullptr;
	ID3D11ShaderResourceView* pShaderResourceViewA = nullptr;
	ID3D11ShaderResourceView* pShaderResourceViewB = nullptr;

	if (FAILED(rDevice.GetD3DDevice()->CreateTexture2D(
		&textureDesc,
		nullptr,
		&pTextureA)))
	{
		goto error;
	}

	if (FAILED(rDevice.GetD3DDevice()->CreateShaderResourceView(
		pTextureA,
		&shaderResourceViewDesc,
		&pShaderResourceViewA)))
	{
		goto error;
	}

	if (FAILED(rDevice.GetD3DDevice()->CreateRenderTargetView(
		pTextureA,
		&renderTargetDesc,
		&pRenderTargetViewA)))
	{
		goto error;
	}

	if (SupportsSimultaneousInputOutput())
	{
		if (FAILED(rDevice.GetD3DDevice()->CreateTexture2D(
			&textureDesc,
			nullptr,
			&pTextureB)))
		{
			goto error;
		}

		if (FAILED(rDevice.GetD3DDevice()->CreateShaderResourceView(
			pTextureB,
			&shaderResourceViewDesc,
			&pShaderResourceViewB)))
		{
			goto error;
		}

		if (FAILED(rDevice.GetD3DDevice()->CreateRenderTargetView(
			pTextureB,
			&renderTargetDesc,
			&pRenderTargetViewB)))
		{
			goto error;
		}
	}
	else
	{
		pRenderTargetViewB = pRenderTargetViewA;
		pRenderTargetViewB->AddRef();
		pShaderResourceViewB = pShaderResourceViewA;
		pShaderResourceViewB->AddRef();
		pTextureB = pTextureA;
		pTextureB->AddRef();
	}

	m_pRenderTargetViewA = pRenderTargetViewA;
	m_pRenderTargetViewB = pRenderTargetViewB;
	m_pShaderResourceViewA = pShaderResourceViewA;
	m_pShaderResourceViewB = pShaderResourceViewB;
	m_pTextureA = pTextureA;
	m_pTextureB = pTextureB;

	return true;

error:
	SafeRelease(pRenderTargetViewB);
	SafeRelease(pRenderTargetViewA);
	SafeRelease(pShaderResourceViewB);
	SafeRelease(pShaderResourceViewA);
	SafeRelease(pTextureB);
	SafeRelease(pTextureA);
	return false;
}

/** Shared destruction of target resources. */
void D3D11RenderTarget::InternalDestroy()
{
	SEOUL_ASSERT(IsRenderThread());

	SafeRelease(m_pRenderTargetViewB);
	SafeRelease(m_pRenderTargetViewA);
	SafeRelease(m_pShaderResourceViewB);
	SafeRelease(m_pShaderResourceViewA);
	SafeRelease(m_pTextureB);
	SafeRelease(m_pTextureA);
}

} // namespace Seoul
