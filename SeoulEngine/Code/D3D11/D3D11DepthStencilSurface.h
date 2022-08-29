/**
 * \file D3D11DepthStencilSurface.h
 * \brief Specialization of DepthStencilSurface for D3D11 rendering on PC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_DEPTH_STENCIL_SURFACE_H
#define D3D11_DEPTH_STENCIL_SURFACE_H

#include "CheckedPtr.h"
#include "DepthStencilSurface.h"
#include "D3D11Util.h"
#include "UnsafeHandle.h"
struct ID3D11DepthStencilView;
struct ID3D11Resource;
struct ID3D11ShaderResourceView;

namespace Seoul
{

class D3D11DepthStencilSurface SEOUL_SEALED : public DepthStencilSurface
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
		// supported in D3D11.
		return UnsafeHandle();
	}
	// /BaseTexture overrides

	virtual void Select() SEOUL_OVERRIDE;
	virtual void Unselect() SEOUL_OVERRIDE;
	virtual void Resolve() SEOUL_OVERRIDE;

	void Reselect();

	/**
	 * @return The underlying IDirect3DSurface9 object of this D3D11DepthStencilSurface.
	 */
	CheckedPtr<ID3D11DepthStencilView> GetView() const
	{
		return m_pDepthStencilView;
	}

private:
	D3D11DepthStencilSurface(const DataStoreTableUtil& configSettings);
	~D3D11DepthStencilSurface();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11DepthStencilSurface);

	friend class D3D11Device;
	friend class DepthStencilSurface;
	CheckedPtr<ID3D11Resource> m_pTexture;
	CheckedPtr<ID3D11DepthStencilView> m_pDepthStencilView;

	SEOUL_DISABLE_COPY(D3D11DepthStencilSurface);
}; // class D3D11DepthStencilSurface

} // namespace Seoul

#endif // include guard
