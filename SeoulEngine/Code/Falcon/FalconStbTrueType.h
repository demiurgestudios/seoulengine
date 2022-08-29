/**
 * \file FalconTrueType.h
 * \brief Internal Falcon header, wraps the stb_truetype library.
 *
 * Do not include outside of .cpp files in the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_STB_TRUE_TYPE_H
#define FALCON_STB_TRUE_TYPE_H

#include "HashTable.h"
#include "MemoryManager.h"

// Necessary to include these here, so that stb_truetype.h does not include them again inside Falcon.
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Seoul::Falcon
{

#	define STBTT_pow(x,y) pow(x,y)
#	define STBTT_sqrt(x) sqrt(x)
#	define STBTT_sort(data,num_items,item_size,compare_func) qsort(data,num_items,item_size,compare_func)
#	define STBTT_ifloor(x) ((int) floor(x))
#	define STBTT_iceil(x) ((int) ceil(x))
#	define STBTT_malloc(x,u) MemoryManager::Allocate((x), MemoryBudgets::Falcon)
#	define STBTT_free(x,u) MemoryManager::Deallocate((x))
#	define STBTT_assert(x) assert(x)
#	define STBTT_strlen(x) strlen(x)
#	define STBTT_memcpy memcpy
#	define STBTT_memset memset
#	include <stb_truetype.h>

// Our equivalent to stbtt_MakeGlyphBitmap, used for generating
// SDF data.
void MakeGlyphBitmapSDF(
	stbtt_fontinfo const* pInfo,
	UInt8* pOutput,
	Int32 iWidth,
	Int32 iHeight,
	Int32 iStride,
	Float fScaleX,
	Float fScaleY,
	Int32 iGlyph);

// Utility, builds a UniChar -> index mapping table for
// all valid UniChars in the font.
typedef HashTable<UniChar, Int32, MemoryBudgets::Falcon> UniCharToIndex;
void GetUniCharToIndexTable(
	stbtt_fontinfo const* pInfo,
	UniCharToIndex& rt);

} // namespace Seoul::Falcon

#endif // include guard
