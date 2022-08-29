/**
 * \file FxStudioUtil.h
 * \brief Utilities to convert to/from FxStudio types and other helpers
 * for interacting with FxStudio.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_UTIL_H
#define FX_STUDIO_UTIL_H

#include "Color.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

/**
 * Convert a raw ARGB Int32 FxStudio color (as returned
 * from, for example, a ColorARGBKeyFrameProperty) to a Seoul
 * engine ColorARGBu8.
 */
inline ColorARGBu8 Int32ColorToColorARGBu8(Int iRawValue)
{
	union
	{
		Int32 in;
		UInt32 out;
	};

	in = iRawValue;

	ColorARGBu8 ret;
	ret.m_Value = out;
	return ret;
}

/**
 * Convert a raw ARGB Int32 FxStudio color (as returned
 * from, for example, a ColorARGBKeyFrameProperty) to a Seoul
 * engine Color4.
 */
inline Color4 Int32ColorToColor4(Int iRawValue)
{
	return Color4(Int32ColorToColorARGBu8(iRawValue));
}

/**
 * Converts an FxStudio FloatRange value to a Seoul::Vector2D value.
 */
inline Vector2D FloatRangeToVector2D(const ::FxStudio::FloatRange& v)
{
	return Vector2D(v.m_fMin, v.m_fMax);
}

/**
 * Helper function, returns true if the FxStudio Property
 * property has the property name pPropertyName, false otherwise.
 */
template <typename T>
Bool IsProperty(const T& property, const char* pPropertyName)
{
	return (strcmp(property.GetPropertyName(), pPropertyName) == 0);
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
