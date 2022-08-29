/**
 * \file D3D11Util.h
 * \brief Common header for D3D11 implementations.
 * This file includes D3D11 headers and defines helper functions
 * and macros for interacting with the D3D11 API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_UTIL_H
#define D3D11_UTIL_H

#include "Geometry.h"
#include "Prereqs.h"
#include "PrimitiveType.h"
#include "SeoulString.h"
#include "Viewport.h"

// Enable extra D3D11 debugging in debug builds if using the debug DirectX runtime.
#if SEOUL_DEBUG
#ifndef D3D_DEBUG_INFO
#define D3D_DEBUG_INFO
#endif
#endif  // SEOUL_DEBUG

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

namespace Seoul
{

const char* GetDXFormatAsString(DXGI_FORMAT f);
const char* GetD3D11ErrorAsString(HRESULT h);

enum class PixelFormat : Int32;

DXGI_FORMAT PixelFormatToD3D(PixelFormat eFormat);
PixelFormat D3DToPixelFormat(DXGI_FORMAT eFormat);

enum class DepthStencilFormat : Int32;

DXGI_FORMAT DepthStencilFormatToD3D(DepthStencilFormat eFormat);
DepthStencilFormat D3DToDepthStencilFormat(DXGI_FORMAT eFormat);

/**
 * Helper function, converts a Seoul engine PrimitiveType enum to
 * a D3D11 PrimitiveType enum.
 */
inline D3D11_PRIMITIVE_TOPOLOGY PrimitiveTypeToD3D11Type(PrimitiveType eType)
{
	switch (eType)
	{
	case PrimitiveType::kPointList: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PrimitiveType::kLineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case PrimitiveType::kLineStrip: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case PrimitiveType::kTriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	default:
		SEOUL_FAIL("Invalid PrimitiveType enum, this is a bug.");
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

static inline Bool operator==(const D3D11_BLEND_DESC& a, const D3D11_BLEND_DESC& b)
{
	return (0 == memcmp(&a, &b, sizeof(a)));
}

static inline Bool operator!=(const D3D11_BLEND_DESC& a, const D3D11_BLEND_DESC& b)
{
	return !(a == b);
}

static inline UInt32 GetHash(const D3D11_BLEND_DESC& a)
{
	return Seoul::GetHash((Byte const*)&a, (UInt32)sizeof(a));
}

/**
 * Used by HashTable as the "null" key value,
 * used as a marker for undefined values and should never
 * be passed in as a valid key.
 */
template <>
struct DefaultHashTableKeyTraits<D3D11_BLEND_DESC>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static D3D11_BLEND_DESC GetNullKey()
	{
		D3D11_BLEND_DESC ret;
		memset(&ret, 0, sizeof(ret));
		return ret;
	}

	static const Bool kCheckHashBeforeEquals = false;
};

static inline Bool operator==(const D3D11_DEPTH_STENCIL_DESC& a, const D3D11_DEPTH_STENCIL_DESC& b)
{
	return (0 == memcmp(&a, &b, sizeof(a)));
}

static inline Bool operator!=(const D3D11_DEPTH_STENCIL_DESC& a, const D3D11_DEPTH_STENCIL_DESC& b)
{
	return !(a == b);
}

static inline UInt32 GetHash(const D3D11_DEPTH_STENCIL_DESC& a)
{
	return Seoul::GetHash((Byte const*)&a, (UInt32)sizeof(a));
}

/**
 * Used by HashTable as the "null" key value,
 * used as a marker for undefined values and should never
 * be passed in as a valid key.
 */
template <>
struct DefaultHashTableKeyTraits<D3D11_DEPTH_STENCIL_DESC>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static D3D11_DEPTH_STENCIL_DESC GetNullKey()
	{
		D3D11_DEPTH_STENCIL_DESC ret;
		memset(&ret, 0, sizeof(ret));
		return ret;
	}

	static const Bool kCheckHashBeforeEquals = false;
};

// Helper macro - if in debug, asserts on a D3D11 API call failure.
// In other builds, performs the operation but does not checking on
// the return value.
#if SEOUL_DEBUG
#	define SEOUL_D3D11_VERIFY(a) SEOUL_ASSERT_MESSAGE(IsRenderThread() && ((a) >= 0), String::Printf(__FUNCTION__ " (%d): \"%s\"", __LINE__, GetD3D11ErrorAsString(a)).CStr())
#else
#	define SEOUL_D3D11_VERIFY(a) ((void)(a))
#endif

/**
 * Helper function, given a Seoul engine viewport structure,
 * returns a DirectX11 viewport structure.
 */
inline static D3D11_VIEWPORT Convert(const Viewport& viewport)
{
	D3D11_VIEWPORT ret;
	ret.TopLeftX = (Float)viewport.m_iViewportX;
	ret.TopLeftY = (Float)viewport.m_iViewportY;
	ret.Width = (Float)viewport.m_iViewportWidth;
	ret.Height = (Float)viewport.m_iViewportHeight;
	ret.MinDepth = 0.0f;
	ret.MaxDepth = 1.0f;
	return ret;
}

inline static RECT Convert(const Rectangle2DInt& rectangle)
{
	RECT ret;
	ret.bottom = rectangle.m_iBottom;
	ret.left = rectangle.m_iLeft;
	ret.right = rectangle.m_iRight;
	ret.top = rectangle.m_iTop;
	return ret;
}

} // namespace Seoul

#endif // include guard
