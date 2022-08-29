/**
 * \file D3D11RenderCommandStreamBuilder.h
 * \brief Specialization of RenderCommandStreamBuilder for the D3D11 graphics system.
 * Handles execution of a command buffer of graphics commands with the D3D11 API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_RENDER_COMMAND_STREAM_BUILDER_H
#define D3D11_RENDER_COMMAND_STREAM_BUILDER_H

#include "RenderCommandStreamBuilder.h"
#include "UnsafeHandle.h"

namespace Seoul
{

// Forward declarations
class D3D11EffectStateManager;
class D3D11VertexFormat;

class D3D11RenderCommandStreamBuilder SEOUL_SEALED : public RenderCommandStreamBuilder
{
public:
	D3D11RenderCommandStreamBuilder(UInt32 zInitialCapacity);
	~D3D11RenderCommandStreamBuilder();

	virtual void ExecuteCommandStream(RenderStats& rStats) SEOUL_OVERRIDE;

private:
	template <typename T>
	Bool ReadEffectPair(ID3DX11Effect*& rpEffect, T*& rp)
	{
		UnsafeHandle hEffect;
		UnsafeHandle hValue;

		if (m_CommandStream.Read(&hEffect, sizeof(hEffect)) &&
			m_CommandStream.Read(&hValue, sizeof(hValue)))
		{
			rpEffect = StaticCast<ID3DX11Effect*>(hEffect);
			rp = StaticCast<T*>(hValue);

			return true;
		}

		return false;
	}

	static UInt32 UpdateTexture(
		ID3D11DeviceContext* pContext,
		BaseTexture* pTexture,
		UInt uLevel,
		const Rectangle2DInt& rectangle,
		Byte const* pSource);

	SEOUL_DISABLE_COPY(D3D11RenderCommandStreamBuilder);
}; // class D3D11RenderCommandStreamBuilder

} // namespace Seoul

#endif // include guard
