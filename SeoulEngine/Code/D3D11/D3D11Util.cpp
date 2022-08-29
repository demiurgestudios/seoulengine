/**
 * \file D3D11Util.cpp
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

#include "DepthStencilFormat.h"
#include "D3D11Util.h"
#include "Texture.h"

namespace Seoul
{

/** Given an HRESULT code, returns a string equivalent of that code.
 */
const char* GetD3D11ErrorAsString(HRESULT h)
{
	switch (h)
	{
	case E_FAIL: return "E_FAIL";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	case S_OK: return "S_OK";
	default:
		return "";
	};
}

/**
 * Converts a PixelFormat enum value to an equivalent D3D11
 * format enum value.
 */
DXGI_FORMAT PixelFormatToD3D(PixelFormat eFormat)
{
	switch (eFormat)
	{
		case PixelFormat::kA8R8G8B8: return DXGI_FORMAT_B8G8R8A8_UNORM; break;
		case PixelFormat::kA8R8G8B8sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; break;
		case PixelFormat::kX8R8G8B8: return DXGI_FORMAT_B8G8R8X8_UNORM; break;
		case PixelFormat::kR5G6B5: return DXGI_FORMAT_B5G6R5_UNORM; break;
		case PixelFormat::kA1R5G5B5: return DXGI_FORMAT_B5G5R5A1_UNORM; break;
		case PixelFormat::kA8: return DXGI_FORMAT_A8_UNORM; break;
		case PixelFormat::kA2B10G10R10: return DXGI_FORMAT_R10G10B10A2_UNORM; break;
		case PixelFormat::kA8B8G8R8: return DXGI_FORMAT_R8G8B8A8_UNORM; break;
		case PixelFormat::kG16R16: return DXGI_FORMAT_R16G16_UNORM; break;
		case PixelFormat::kA16B16G16R16: return DXGI_FORMAT_R16G16B16A16_UNORM; break;
		case PixelFormat::kR16F: return DXGI_FORMAT_R16_FLOAT; break;
		case PixelFormat::kD16I: return DXGI_FORMAT_D16_UNORM; break;
		case PixelFormat::kG16R16F: return DXGI_FORMAT_R16G16_FLOAT; break;
		case PixelFormat::kA16B16G16R16F: return DXGI_FORMAT_R16G16B16A16_FLOAT; break;
		case PixelFormat::kR32F: return DXGI_FORMAT_R32_FLOAT; break;
		case PixelFormat::kG32R32F: return DXGI_FORMAT_R32G32_FLOAT; break;
		case PixelFormat::kA32B32G32R32F: return DXGI_FORMAT_R32G32B32A32_FLOAT; break;
		case PixelFormat::kDXT1: return DXGI_FORMAT_BC1_UNORM; break;
		case PixelFormat::kDXT2: return DXGI_FORMAT_BC2_UNORM; break;
		case PixelFormat::kDXT3: return DXGI_FORMAT_BC2_UNORM; break;
		case PixelFormat::kDXT4: return DXGI_FORMAT_BC3_UNORM; break;
		case PixelFormat::kDXT5: return DXGI_FORMAT_BC3_UNORM; break;

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	};
}

/**
 * Converts a D3D11 format enum value to an equivalent pixel format
 *  enum value.
 */
PixelFormat D3DToPixelFormat(DXGI_FORMAT eFormat)
{
	switch (eFormat)
	{
		case DXGI_FORMAT_B8G8R8A8_UNORM: return PixelFormat::kA8R8G8B8; break;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return PixelFormat::kA8R8G8B8sRGB; break;
		case DXGI_FORMAT_B8G8R8X8_UNORM: return PixelFormat::kX8R8G8B8; break;
		case DXGI_FORMAT_B5G6R5_UNORM: return PixelFormat::kR5G6B5; break;
		case DXGI_FORMAT_B5G5R5A1_UNORM: return PixelFormat::kA1R5G5B5; break;
		case DXGI_FORMAT_A8_UNORM: return PixelFormat::kA8; break;
		case DXGI_FORMAT_R10G10B10A2_UNORM: return PixelFormat::kA2B10G10R10; break;
		case DXGI_FORMAT_R8G8B8A8_UNORM: return PixelFormat::kA8B8G8R8; break;
		case DXGI_FORMAT_R16G16_UNORM: return PixelFormat::kG16R16; break;
		case DXGI_FORMAT_R16G16B16A16_UNORM: return PixelFormat::kA16B16G16R16; break;
		case DXGI_FORMAT_R16_FLOAT: return PixelFormat::kR16F; break;
		case DXGI_FORMAT_D16_UNORM: return PixelFormat::kD16I; break;
		case DXGI_FORMAT_R16G16_FLOAT: return PixelFormat::kG16R16F; break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT: return PixelFormat::kA16B16G16R16F; break;
		case DXGI_FORMAT_R32_FLOAT: return PixelFormat::kR32F; break;
		case DXGI_FORMAT_R32G32_FLOAT: return PixelFormat::kG32R32F; break;
		case DXGI_FORMAT_R32G32B32A32_FLOAT: return PixelFormat::kA32B32G32R32F; break;
		case DXGI_FORMAT_BC1_UNORM: return PixelFormat::kDXT1; break;
		case DXGI_FORMAT_BC2_UNORM: return PixelFormat::kDXT3; break;
		case DXGI_FORMAT_BC3_UNORM: return PixelFormat::kDXT5; break;

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return PixelFormat::kA8R8G8B8;
	};
}

/**
 * Given a DepthStencilSurface enum value, returns an equivalent
 * D3D11 format enum value.
 */
DXGI_FORMAT DepthStencilFormatToD3D(DepthStencilFormat eFormat)
{
	switch (eFormat)
	{
	case DepthStencilFormat::kD16_LOCKABLE: return DXGI_FORMAT_D16_UNORM; break;
	case DepthStencilFormat::kD32: return DXGI_FORMAT_D32_FLOAT; break;
	case DepthStencilFormat::kD24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT; break;
	case DepthStencilFormat::kD24FS8: return DXGI_FORMAT_D24_UNORM_S8_UINT; break;
	case DepthStencilFormat::kD24X8: return DXGI_FORMAT_D24_UNORM_S8_UINT; break;
	case DepthStencilFormat::kD16: return DXGI_FORMAT_D16_UNORM; break;

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	};
}

/**
 * Given a D3D11 format enum value, returns an equivalent
 * D3D11 format enum value.
 */
DepthStencilFormat D3DToDepthStencilFormat(DXGI_FORMAT eFormat)
{
	switch (eFormat)
	{
	case DXGI_FORMAT_D32_FLOAT: return DepthStencilFormat::kD32; break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT: return DepthStencilFormat::kD24S8; break;
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return DepthStencilFormat::kD24X8; break;
	case DXGI_FORMAT_D16_UNORM: return DepthStencilFormat::kD16; break;

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return DepthStencilFormat::kD24X8;
	}
}

} // namespace Seoul
