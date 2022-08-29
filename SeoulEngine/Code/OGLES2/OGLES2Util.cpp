/**
 * \file OGLES2Util.cpp
 * \brief Implementation of common functions for interacting with OGLES2,
 * primarily format and constant conversion.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "OGLES2Util.h"

namespace Seoul
{

GLenum PixelFormatToOpenGlInternalFormat(PixelFormat eFormat)
{
	// On iOS, the internal format for BGRA is RGBA.
	// On iOS, we use GL_RGB for XBGR, since we can depend on the support. On Android, some
	// devices do not accept GL_RGB as a framebuffer format, so we use GL_RGBA to be safe, even
	// though it will advertise that we need an alpha channel even though it will not be used.
#if SEOUL_PLATFORM_IOS
	static const GLenum kInternalFormatARGB = GL_RGBA;
	static const GLenum kInternalFormatXBGR = GL_RGB;
#else
	static const GLenum kInternalFormatARGB = GL_BGRA_EXT;
	static const GLenum kInternalFormatXBGR = GL_RGBA;
#endif

	switch (eFormat)
	{
		case PixelFormat::kR8G8B8: return GL_RGB; break;
		case PixelFormat::kA8R8G8B8: return kInternalFormatARGB; break;
		case PixelFormat::kA8R8G8B8sRGB: return kInternalFormatARGB; break;
		case PixelFormat::kX8R8G8B8: return kInternalFormatARGB; break;
		case PixelFormat::kR5G6B5: return GL_RGB; break;
		case PixelFormat::kX1R5G5B5: return kInternalFormatARGB; break;
		case PixelFormat::kA1R5G5B5: return kInternalFormatARGB; break;
		case PixelFormat::kA4R4G4B4: return kInternalFormatARGB; break;
		case PixelFormat::kR3G3B2: return GL_RGB; break;
		case PixelFormat::kA8: return GL_ALPHA; break;
		case PixelFormat::kA8L8: return GL_LUMINANCE_ALPHA; break;
		//case PixelFormat::kP8: return GL_P; break;
		case PixelFormat::kX4R4G4B4: return kInternalFormatARGB; break;
		case PixelFormat::kA2B10G10R10: return GL_RGBA; break;
		case PixelFormat::kA8B8G8R8: return GL_RGBA; break;
		case PixelFormat::kX8B8G8R8: return kInternalFormatXBGR; break;
		case PixelFormat::kG16R16: return GL_RGBA; break;
		case PixelFormat::kA2R10G10B10: return kInternalFormatARGB; break;
		case PixelFormat::kA16B16G16R16: return GL_RGBA; break;
		case PixelFormat::kR16F: return GL_RGBA; break;
		case PixelFormat::kD16I: return GL_DEPTH_COMPONENT; break;
		case PixelFormat::kG16R16F: return GL_RGBA; break;
		case PixelFormat::kA16B16G16R16F: return GL_RGBA; break;
		case PixelFormat::kR32F: return GL_RGBA; break;
		case PixelFormat::kG32R32F: return GL_RGBA; break;
		case PixelFormat::kA32B32G32R32F: return GL_RGBA; break;
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID
		case PixelFormat::kDXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
		case PixelFormat::kDXT2: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
		case PixelFormat::kDXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
		case PixelFormat::kDXT4: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
		case PixelFormat::kDXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
#endif
#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID
		case PixelFormat::kPVRTC_RGB_4BPPV1: return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; break;
		case PixelFormat::kPVRTC_RGB_2BPPV1: return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG; break;
		case PixelFormat::kPVRTC_RGBA_4BPPV1: return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; break;
		case PixelFormat::kPVRTC_RGBA_2BPPV1: return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG; break;
#endif
#if SEOUL_PLATFORM_ANDROID
		case PixelFormat::kETC1_RGB8: return GL_ETC1_RGB8_OES; break;
#elif SEOUL_PLATFORM_IOS
		case PixelFormat::kETC1_RGB8: return GL_COMPRESSED_RGB8_ETC2; break;
#endif

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return GL_INVALID_ENUM;
	};
}

GLenum PixelFormatToOpenGlFormat(PixelFormat eFormat)
{
	// On iOS, we use GL_RGB for XBGR, since we can depend on the support. On Android, some
	// devices do not accept GL_RGB as a framebuffer format, so we use GL_RGBA to be safe, even
	// though it will advertise that we need an alpha channel even though it will not be used.
#if SEOUL_PLATFORM_IOS
	static const GLenum kFormatXBGR = GL_RGB;
#else
	static const GLenum kFormatXBGR = GL_RGBA;
#endif

	switch (eFormat)
	{
		//case PixelFormat::kR8G8B8: return GL_BGR_EXT; break;
		case PixelFormat::kA8R8G8B8: return GL_BGRA_EXT; break;
		case PixelFormat::kA8R8G8B8sRGB: return GL_BGRA_EXT; break;
		case PixelFormat::kX8R8G8B8: return GL_BGRA_EXT; break;
		//case PixelFormat::kR5G6B5: return GL_BGR_EXT; break;
		case PixelFormat::kX1R5G5B5: return GL_BGRA_EXT; break;
		case PixelFormat::kA1R5G5B5: return GL_BGRA_EXT; break;
		case PixelFormat::kA4R4G4B4: return GL_BGRA_EXT; break;
		//case PixelFormat::kR3G3B2: return GL_BGR_EXT; break;
		case PixelFormat::kA8: return GL_ALPHA; break;
		case PixelFormat::kA8L8: return GL_LUMINANCE_ALPHA; break;
		//case PixelFormat::kP8: return GL_P; break;
		case PixelFormat::kX4R4G4B4: return GL_BGRA_EXT; break;
		case PixelFormat::kA2B10G10R10: return GL_RGBA; break;
		case PixelFormat::kA8B8G8R8: return GL_RGBA; break;
		case PixelFormat::kX8B8G8R8: return kFormatXBGR; break;
		case PixelFormat::kG16R16: return GL_RGBA; break;
		case PixelFormat::kA2R10G10B10: return GL_BGRA_EXT; break;
		case PixelFormat::kA16B16G16R16: return GL_RGBA; break;
		case PixelFormat::kR16F: return GL_RGBA; break;
		case PixelFormat::kD16I: return GL_DEPTH_COMPONENT; break;
		case PixelFormat::kG16R16F: return GL_RGBA; break;
		case PixelFormat::kA16B16G16R16F: return GL_RGBA; break;
		case PixelFormat::kR32F: return GL_RGBA; break;
		case PixelFormat::kG32R32F: return GL_RGBA; break;
		case PixelFormat::kA32B32G32R32F: return GL_RGBA; break;
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID
		case PixelFormat::kDXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
		case PixelFormat::kDXT2: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
		case PixelFormat::kDXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
		case PixelFormat::kDXT4: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
		case PixelFormat::kDXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
#endif
#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID
		case PixelFormat::kPVRTC_RGB_4BPPV1: return GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; break;
		case PixelFormat::kPVRTC_RGB_2BPPV1: return GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG; break;
		case PixelFormat::kPVRTC_RGBA_4BPPV1: return GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; break;
		case PixelFormat::kPVRTC_RGBA_2BPPV1: return GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG; break;
#endif
#if SEOUL_PLATFORM_ANDROID
		case PixelFormat::kETC1_RGB8: return GL_ETC1_RGB8_OES; break;
#elif SEOUL_PLATFORM_IOS
		case PixelFormat::kETC1_RGB8: return GL_COMPRESSED_RGB8_ETC2; break;
#endif

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return GL_INVALID_ENUM;
	};
}

GLenum PixelFormatToOpenGlElementType(PixelFormat eFormat)
{
	switch (eFormat)
	{
		case PixelFormat::kR8G8B8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kA8R8G8B8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kA8R8G8B8sRGB: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kX8R8G8B8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kR5G6B5: return GL_UNSIGNED_SHORT_5_6_5; break;
		case PixelFormat::kX1R5G5B5: return GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT; break;
		case PixelFormat::kA1R5G5B5: return GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT; break;
		case PixelFormat::kA4R4G4B4: return GL_UNSIGNED_SHORT_4_4_4_4; break;
		// case PixelFormat::kR3G3B2: return GL_UNSIGNED_BYTE_3_3_2; break;
		case PixelFormat::kA8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kA8L8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kP8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kX4R4G4B4: return GL_UNSIGNED_SHORT_4_4_4_4; break;
		//case PixelFormat::kA2B10G10R10: return GL_EXT_texture_type_10_10_10_2_REV; break;
		case PixelFormat::kA8B8G8R8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kX8B8G8R8: return GL_UNSIGNED_BYTE; break;
		case PixelFormat::kG16R16: return GL_UNSIGNED_SHORT; break;
		//case PixelFormat::kA2R10G10B10: return GL_EXT_texture_type_2_10_10_10_REV; break;
		case PixelFormat::kA16B16G16R16: return GL_UNSIGNED_SHORT; break;
		case PixelFormat::kR16F: return GL_HALF_FLOAT_OES; break;
		case PixelFormat::kD16I: return GL_UNSIGNED_SHORT; break;
		case PixelFormat::kG16R16F: return GL_HALF_FLOAT_OES; break;
		case PixelFormat::kA16B16G16R16F: return GL_HALF_FLOAT_OES; break;
		case PixelFormat::kR32F: return GL_FLOAT; break;
		case PixelFormat::kG32R32F: return GL_FLOAT; break;
		case PixelFormat::kA32B32G32R32F: return GL_FLOAT; break;

	default:
		SEOUL_FAIL("Switch statement enum mismatch");
		return GL_INVALID_ENUM;
	};
}

GLenum DepthStencilFormatToOpenGlFormat(DepthStencilFormat eFormat)
{
	switch (eFormat)
	{
	case DepthStencilFormat::kD24S8: // fall-through
	case DepthStencilFormat::kD24FS8: // fall-through
	case DepthStencilFormat::kD24X8: // fall-through
	case DepthStencilFormat::kD16S8:
		return GL_DEPTH24_STENCIL8_OES;

	case DepthStencilFormat::kD16_LOCKABLE: // fall-through
	case DepthStencilFormat::kD16:
		return GL_DEPTH_COMPONENT16;

	case DepthStencilFormat::kD32:
		return GL_DEPTH_COMPONENT32_OES;

	case DepthStencilFormat::kD15S1: // fall-through
	case DepthStencilFormat::kD24X4S4: // fall-through
	default:
		return GL_INVALID_ENUM;
	};
}

GLenum DepthStencilFormatToOpenGlDepthFormat(DepthStencilFormat eFormat)
{
	switch (eFormat)
	{
	case DepthStencilFormat::kD24S8: // fall-through
	case DepthStencilFormat::kD24FS8: // fall-through
	case DepthStencilFormat::kD24X8: // fall-through
		return GL_DEPTH_COMPONENT24_OES;

	case DepthStencilFormat::kD16_LOCKABLE: // fall-through
	case DepthStencilFormat::kD16:
	case DepthStencilFormat::kD16S8:
		return GL_DEPTH_COMPONENT16;

	case DepthStencilFormat::kD32:
		return GL_DEPTH_COMPONENT32_OES;

	case DepthStencilFormat::kD15S1: // fall-through
	case DepthStencilFormat::kD24X4S4: // fall-through
	default:
		return GL_INVALID_ENUM;
	};
}

GLenum DepthStencilFormatToOpenGlStencilFormat(DepthStencilFormat eFormat)
{
	switch (eFormat)
	{
	case DepthStencilFormat::kD24S8: // fall-through
	case DepthStencilFormat::kD24FS8: // fall-through
	case DepthStencilFormat::kD24X8: // fall-through
	case DepthStencilFormat::kD16S8:
		return GL_STENCIL_INDEX8;

	case DepthStencilFormat::kD16_LOCKABLE: // fall-through
	case DepthStencilFormat::kD16: // fall-through
	case DepthStencilFormat::kD32: // fall-through
	case DepthStencilFormat::kD15S1: // fall-through
	case DepthStencilFormat::kD24X4S4: // fall-through
	default:
		return GL_INVALID_ENUM;
	};
}

#define SEOUL_OPENGL_ERROR(name) \
	name: return #name

const char* GetOpenGlESErrorString(Int32 errorCode)
{
	switch (errorCode)
	{
	case SEOUL_OPENGL_ERROR(GL_NO_ERROR); break;
	case SEOUL_OPENGL_ERROR(GL_INVALID_ENUM); break;
	case SEOUL_OPENGL_ERROR(GL_INVALID_VALUE); break;
	case SEOUL_OPENGL_ERROR(GL_INVALID_OPERATION); break;
	case SEOUL_OPENGL_ERROR(GL_OUT_OF_MEMORY); break;
	case SEOUL_OPENGL_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION); break;
	default:
		return "Unknown Error";
	};
}

#undef SEOUL_OPENGL_ERROR

} // namespace Seoul
