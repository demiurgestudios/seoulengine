/**
 * \file OGLES2Util.h
 * \brief Implementation of common functions for interacting with OGLES2,
 * primarily format and constant conversion.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_UTIL_H
#define OGLES2_UTIL_H

#include "DepthStencilFormat.h"
#include "Geometry.h"
#include "PixelFormat.h"
#include "Prereqs.h"
#include "SeoulString.h"
#include "VertexElement.h"

#if SEOUL_PLATFORM_WINDOWS
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#endif

#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif SEOUL_PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else
#error "Define for this platform."
#endif

// Supported on Tegra chipsets on Android, but must be manually defined.
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE
#define GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE 0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE
#define GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE 0x83F3
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE
#endif

// ETC2 format from OpenGL ES 3.
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif

// Texture configuration from OpenGL ES 3.
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

namespace Seoul
{

GLenum PixelFormatToOpenGlInternalFormat(PixelFormat eFormat);
GLenum PixelFormatToOpenGlFormat(PixelFormat eFormat);
GLenum PixelFormatToOpenGlElementType(PixelFormat eFormat);

GLenum DepthStencilFormatToOpenGlFormat(DepthStencilFormat eFormat);
GLenum DepthStencilFormatToOpenGlDepthFormat(DepthStencilFormat eFormat);
GLenum DepthStencilFormatToOpenGlStencilFormat(DepthStencilFormat eFormat);

/**
 * @return The OpenGlES numeric index for a vertex attribute which
 * matches the vertex attribute defined by VertexFormat element
 * element.
 */
inline static UInt8 GetVertexDataIndex(const VertexElement& element)
{
	switch (element.m_Usage)
	{
	case VertexElement::UsagePosition: return 0u;
	case VertexElement::UsageBlendWeight: return 12u;
	case VertexElement::UsageBlendIndices: return 15u;
	case VertexElement::UsageNormal: return 1u;
	case VertexElement::UsagePSize: return 6u;
	case VertexElement::UsageTexcoord: return (4u + Clamp(element.m_UsageIndex, 0u, 7u));
	case VertexElement::UsageTangent: return 13u;
	case VertexElement::UsageBinormal: return 14u;
	case VertexElement::UsageTessfactor: return 5u;
	case VertexElement::UsageColor: return (2u + Clamp(element.m_UsageIndex, 0u, 1u));
	case VertexElement::UsageFog: return 5u; // shares the same attribute with tessfactor.

	case VertexElement::UsagePositionT: // fall-through
	case VertexElement::UsageDepth: // fall-through
	case VertexElement::UsageSample: // fall-through
	default:
		// Unsupported attribute
		return UInt8Max;
	};
}

/**
 * The number of individual components that a vertex element
 * has on under OGLES2.
 */
inline static UInt8 GetVertexElementComponentCount(const VertexElement& element)
{
	// Handle these specially - unlike other platforms, these three types
	// are considered 4 component values and are not endian swapped (on other
	// platforms, they are all treated as a single UInt32 value that must be
	// endian swapped).
	if (VertexElement::TypeColor == element.m_Type ||
		VertexElement::TypeUByte4 == element.m_Type ||
		VertexElement::TypeUByte4N == element.m_Type)
	{
		return 4u;
	}
	else
	{
		return VertexElement::ComponentCountFromType(element.m_Type);
	}
}

/**
 * @return GL_TRUE if element describes a normalized vertex element type,
 * false otherwise.
 */
inline static GLboolean GetVertexElementIsNormalized(const VertexElement& element)
{
	switch (element.m_Type)
	{
	case VertexElement::TypeColor: // fall-through
	case VertexElement::TypeUByte4N: // fall-through
	case VertexElement::TypeShort2N: // fall-through
	case VertexElement::TypeShort4N: // fall-through
	case VertexElement::TypeDec3N: // fall-through
	case VertexElement::TypeUShort2N: // fall-through
	case VertexElement::TypeUShort4N:
		return GL_TRUE;
	default:
		return GL_FALSE;
	};
}

/**
 * The GLenum code that matches the value type of the
 * data defined by element.
 */
inline static GLenum GetVertexElementType(const VertexElement& element)
{
	switch (element.m_Type)
	{
	case VertexElement::TypeFloat1: return GL_FLOAT;
	case VertexElement::TypeFloat2: return GL_FLOAT;
	case VertexElement::TypeFloat3: return GL_FLOAT;
	case VertexElement::TypeFloat4: return GL_FLOAT;
	case VertexElement::TypeColor: return GL_UNSIGNED_BYTE;
	case VertexElement::TypeUByte4: return GL_UNSIGNED_BYTE;
	case VertexElement::TypeShort2: return GL_SHORT;
	case VertexElement::TypeShort4: return GL_SHORT;
	case VertexElement::TypeUByte4N: return GL_UNSIGNED_BYTE;
	case VertexElement::TypeShort2N: return GL_SHORT;
	case VertexElement::TypeShort4N: return GL_SHORT;
	case VertexElement::TypeFloat16_2: return GL_SHORT; // TODO: Placeholder, remove support.
	case VertexElement::TypeFloat16_4: return GL_SHORT; // TODO: Placeholder, remove support.
	case VertexElement::TypeUShort2N: return GL_UNSIGNED_SHORT;
	case VertexElement::TypeUShort4N: return GL_UNSIGNED_SHORT;

	case VertexElement::TypeDec3N: // fall-through
	case VertexElement::TypeUDec3: // fall-through
	case VertexElement::TypeUnused: // fall-through
	default:
		return 0u;
	};
}

const char* GetOpenGlESErrorString(Int32 errorCode);

inline void OGLES2PreVerify(char const* sFunction, int iLine)
{
	GLenum err = glGetError();
	if (GL_NO_ERROR != err)
	{
		SEOUL_FAIL(String::Printf("%s (%d, pre): \"%s\"", sFunction, iLine, GetOpenGlESErrorString(err)).CStr());
	}
}

inline void OGLES2Verify(char const* sFunction, int iLine)
{
	GLenum err = glGetError();
	if (GL_NO_ERROR != err)
	{
		SEOUL_FAIL(String::Printf("%s (%d, post): \"%s\"", sFunction, iLine, GetOpenGlESErrorString(err)).CStr());
	}
}

#if SEOUL_DEBUG
#define SEOUL_OGLES2_VERIFY(a) OGLES2PreVerify(__FUNCTION__, __LINE__); (a); OGLES2Verify(__FUNCTION__, __LINE__)
#else
#define SEOUL_OGLES2_VERIFY(a) (a)
#endif

inline void OGLES2ClearError()
{
	// See documentation for glGetError() for why this is necessary.
	while (GL_NO_ERROR != glGetError()) ;
}

// Utility, replaces nullptr return values with "".
static inline Byte const* SafeGlGetString(GLenum eName)
{
	auto sReturn = (Byte const*)glGetString(eName);
	if (nullptr == sReturn) { sReturn = ""; }
	return sReturn;
}

} // namespace Seoul

// Extension declarations.
extern "C" { typedef void (*PopGroupMarkerEXT)(); }
extern "C" { typedef void (*PushGroupMarkerEXT)(GLsizei length, const GLchar* marker); }

#endif // include guard
