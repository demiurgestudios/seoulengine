/**
 * \file D3D11RenderTarget.h
 * \brief Specialization of RenderTarget for the D3D11 platform. RenderTarget
 * encapsulates a renderable color surface.
 *
 * In D3D11, RenderTarget can potentially encapsulate
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
#ifndef D3D11_RENDER_TARGET_H
#define D3D11_RENDER_TARGET_H

#include "RenderTarget.h"
#include "UnsafeHandle.h"
struct ID3D11Resource;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace Seoul
{

/**
 * D3D11RenderTarget encapsulates a renderable color buffer on D3D11,
 * that can also be sampled as a texture.
 */
class D3D11RenderTarget SEOUL_SEALED : public RenderTarget
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
		return UnsafeHandle((ID3D11ShaderResourceView*)m_pShaderResourceViewB.Get());
	}
	// /BaseTexture overrides

	// RenderTarget overrides
	virtual void Select() SEOUL_OVERRIDE;
	virtual void Unselect() SEOUL_OVERRIDE;
	virtual void Resolve() SEOUL_OVERRIDE;
	// /RenderTarget overrides

	void Reselect();

	/**
	 * @return The underlying IDirect3DSurface9 object of this D3D11RenderTarget.
	 */
	CheckedPtr<ID3D11RenderTargetView> GetView() const
	{
		return m_pRenderTargetViewA;
	}

private:
	D3D11RenderTarget(const DataStoreTableUtil& configSettings);
	virtual ~D3D11RenderTarget();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11RenderTarget);

	Bool InternalCreate();
	void InternalDestroy();

	friend class D3D11Device;
	friend class D3D11RenderCommandStreamBuilder;
	friend class RenderTarget;
	CheckedPtr<ID3D11Resource> m_pTextureA;
	CheckedPtr<ID3D11Resource> m_pTextureB;
	CheckedPtr<ID3D11ShaderResourceView> m_pShaderResourceViewA;
	CheckedPtr<ID3D11ShaderResourceView> m_pShaderResourceViewB;
	CheckedPtr<ID3D11RenderTargetView> m_pRenderTargetViewA;
	CheckedPtr<ID3D11RenderTargetView> m_pRenderTargetViewB;

	SEOUL_DISABLE_COPY(D3D11RenderTarget);
}; // class D3D11RenderTarget

} // namespace Seoul

#endif // include guard
