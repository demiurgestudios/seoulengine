/**
 * \file Color.cpp
 * \brief Structures representing a 4 component, 32-bit float per
 * component color value and a 4 component, 8-bit unsigned int per
 * component color value.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Color.h"

namespace Seoul
{

/**
 * Utility used during static initialization to construct the table used
 * to convert sRGB non-linear gamma color values into lRGB linear color values.
 */
static inline FixedArray< Float, kSRGBToLinearTableSize> InternalStaticGenerateSRGBToLinearTable()
{
	FixedArray< Float, kSRGBToLinearTableSize> aReturn;

	// Generate the table used to convert sRGB [0, 255] byte values
	// into the equivalent linear space float value.
	for (UInt32 i = 0; i < kSRGBToLinearTableSize; ++i)
	{
		aReturn[i] = SRGBDeGamma((UInt8)i);
	}

	return aReturn;
}

/**
 * Utility used during static initialization to construct the table used
 * to convert lRGB linear gamma color values into sRGB non-linear color values.
 */
static inline FixedArray< UInt8, kLinearToSRGBTableSize > InternalStaticGenerateLinearToSRGBTable()
{
	FixedArray< UInt8, kLinearToSRGBTableSize > aReturn;

	// Generate the table used to convert lRGB [0.0, 1.0] float values
	// into the equivalent sRGB byte value.
	for (UInt32 i = 0; i < kLinearToSRGBTableSize; ++i)
	{
		aReturn[i] = SRGBGamma((Float)i / (Float)(kLinearToSRGBTableSize - 1));
	}

	return aReturn;
}

/** Table used for fast sRGB -> lRGB lookups */
FixedArray< Float, kSRGBToLinearTableSize> g_kaSRGBToLinear(InternalStaticGenerateSRGBToLinearTable());

/** Table used for fast lRGB -> sRGB lookups */
FixedArray< UInt8, kLinearToSRGBTableSize> g_kaLinearToSRGB(InternalStaticGenerateLinearToSRGBTable());

} // namespace Seoul
