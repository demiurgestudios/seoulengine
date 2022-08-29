/**
 * \file ImageWrite.cpp
 * \brief Utility functions for writing image data of various formats
 * (e.g. PNG, TGA, etc.).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ImageWrite.h"

// Necessary to include these here, so that stb_image.h does not include them again inside the
// Seoul namespace.
#include <assert.h>
#ifdef _MSC_VER
#include <emmintrin.h>
#include <intrin.h>
#endif
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Seoul
{

#ifdef __clang__
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wunused-function"
#endif

#	define STB_IMAGE_RESIZE_IMPLEMENTATION
#	define STB_IMAGE_RESIZE_STATIC
#	include <stb_image_resize.h>

#	define STB_IMAGE_WRITE_IMPLEMENTATION
#	define STB_IMAGE_WRITE_STATIC
#	define STBI_WRITE_NO_STDIO
#	define STBIW_ASSERT(x) SEOUL_ASSERT(x)
#	define STBIW_MALLOC(sz) Seoul::MemoryManager::Allocate((size_t)(sz), MemoryBudgets::Io)
#	define STBIW_REALLOC(p,sz) Seoul::MemoryManager::Reallocate((p), (size_t)(sz), MemoryBudgets::Io)
#	define STBIW_FREE(p) Seoul::MemoryManager::Deallocate((p))
#	ifndef _MSC_VER
#		define STBIW_NO_SIMD 1
#	else
#		define STBI_MSC_SECURE_CRT 1
#	endif
#	include <stb_image_write.h>

#ifdef __clang__
#	pragma clang diagnostic pop
#endif

namespace
{

struct Context SEOUL_SEALED
{
	Context(SyncFile& rFile)
		: m_rFile(rFile)
		, m_bError(false)
	{
	}

	SyncFile& m_rFile;
	Bool m_bError;
};

} // namespace anonymous

static void WriteFunc(void* pContext, void* pData, int iSize)
{
	auto& context = *((Context*)pContext);
	if (context.m_bError)
	{
		return;
	}

	if (iSize < 0)
	{
		context.m_bError = true;
		return;
	}

	if ((UInt32)iSize != context.m_rFile.WriteRawData(pData, (UInt32)iSize))
	{
		context.m_bError = true;
		return;
	}
}

Bool ImageWritePng(
	Int32 iWidth,
	Int32 iHeight,
	Int32 iComponents,
	void const* pData, 
	Int32 iStrideInBytes, 
	SyncFile& rFile)
{
	Context context(rFile);
	return (0 != stbi_write_png_to_func(WriteFunc, &context, iWidth, iHeight, iComponents, pData, iStrideInBytes)) && !context.m_bError;
}

Bool ImageResizeAndWritePng(
	Int32 iWidth,
	Int32 iHeight,
	Int32 iComponents,
	void const* pData, 
	Int32 iStrideInBytes, 
	Int32 iOutWidth,
	Int32 iOutHeight, 
	SyncFile& rFile)
{
	Bool bCleanup = false;
	if (iWidth != iOutWidth || iHeight != iOutHeight)
	{
		void* pNewData = MemoryManager::Allocate(iOutWidth * iOutHeight * iComponents, MemoryBudgets::Rendering);
		bCleanup = true;

		if (0 == stbir__resize_arbitrary(
			nullptr,
			(UInt8 const*)pData,
			iWidth,
			iHeight,
			iStrideInBytes,
			(UInt8*)pNewData,
			iOutWidth,
			iOutHeight,
			(iOutWidth * iComponents),
			0.0f, 0.0f, 1.0f, 1.0f,
			nullptr,
			iComponents,
			3,
			0,
			STBIR_TYPE_UINT8,
			STBIR_FILTER_CATMULLROM,
			STBIR_FILTER_CATMULLROM,
			STBIR_EDGE_CLAMP,
			STBIR_EDGE_CLAMP,
			STBIR_COLORSPACE_SRGB))
		{
			MemoryManager::Deallocate(pNewData);
			pNewData = nullptr;
			return false;
		}

		pData = pNewData;
		iStrideInBytes = (iComponents * iOutWidth);
	}

	auto const b = ImageWritePng(iOutWidth, iOutHeight, iComponents, pData, iStrideInBytes, rFile);

	if (bCleanup)
	{
		MemoryManager::Deallocate((void*)pData);
		pData = nullptr;
	}

	return b;
}

} // namespace Seoul
