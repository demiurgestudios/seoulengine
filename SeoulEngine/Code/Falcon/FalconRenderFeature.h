/**
 * \file FalconRenderFeature.h
 * \brief A Render::Feature is a gradually more complex
 * (and thus more costly at runtime) definition of the
 * material that is necessary to render a Falcon batch.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDER_FEATURE_H
#define FALCON_RENDER_FEATURE_H

#include "Prereqs.h"

namespace Seoul::Falcon
{

namespace Render
{

namespace Feature
{
	enum Enum
	{
		// No specific modes needed.
		kNone = 0,

		// Draw operation needs color multiply.
		kColorMultiply = (1 << 0),

		// Draw operation needs color add.
		kColorAdd = (1 << 1),

		// Draw operation needs alpha shaping (for SDF rendering).
		kAlphaShape = (1 << 2),
		
		// Draw operation needs a detail texture. Detail texture
		// is currently supported only be text when it is draw
		// with a "face" texture (e.g. a rainbow pattern layered
		// across the face of the text).
		kDetail = (1 << 3),

		// Draw operation requires mode switch to SrcBlend = One, DstBlend = InvSrcColor.
		kExtended_InvSrcAlpha_One      =  (1 << 4), // Not a bit selector, numbers that use a subset of bits.
		kExtended_InvSrcColor_One      =  (2 << 4),
		kExtended_One_InvSrcColor      =  (3 << 4),
		kExtended_One_SrcAlpha         =  (4 << 4),
		kExtended_One_SrcColor         =  (5 << 4),
		kExtended_SrcAlpha_InvSrcAlpha =  (6 << 4),
		kExtended_SrcAlpha_InvSrcColor =  (7 << 4),
		kExtended_SrcAlpha_One         =  (8 << 4),
		kExtended_SrcAlpha_SrcAlpha    =  (9 << 4),
		kExtended_SrcColor_InvSrcAlpha = (10 << 4),
		kExtended_SrcColor_InvSrcColor = (11 << 4),
		kExtended_SrcColor_One         = (12 << 4),
		kExtended_Zero_InvSrcColor     = (13 << 4),
		kExtended_Zero_SrcColor        = (14 << 4),
		kExtended_ColorAlphaShape      = (15 << 4),

		// Combinations - we reserve 4 bits for extended modes.
		EXTENDED_MASK = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7),
		EXTENDED_SHIFT = 4,

		// For allocations.
		EXTENDED_MIN = kExtended_InvSrcAlpha_One,
		EXTENDED_MAX = kExtended_ColorAlphaShape,
		EXTENDED_COUNT = (EXTENDED_MAX >> EXTENDED_SHIFT) - (EXTENDED_MIN >> EXTENDED_SHIFT) + 1,
	};
	SEOUL_STATIC_ASSERT(EXTENDED_COUNT <= 16); // Sanity, must fit in 4 bits.
	SEOUL_STATIC_ASSERT(EXTENDED_MASK >> 4 == 15);
	
	/** For ordered conversion of extended modes into other lookups (e.g. state name). */
	static inline UInt32 ExtendedToIndex(UInt32 uBits)
	{
		// Narrow and shift.
		auto const uAdjusted = (uBits & EXTENDED_MASK) >> EXTENDED_SHIFT;
		auto const uReturn = (uAdjusted - (EXTENDED_MIN >> EXTENDED_SHIFT));
		
		return uReturn;
	}
} // namespace Feature

} // namespace Render

} // namespace Seoul::Falcon

#endif // include guard
