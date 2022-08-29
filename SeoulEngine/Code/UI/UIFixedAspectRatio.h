/**
 * \file UIFixedAspectRatio.h
 * \brief Enum of aspect ratio configurations.
 * Convenience and representation for developers, essential
 * UI functionality supports arbitrary aspect ratios.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_FIXED_ASPECT_RATIO_H
#define UI_FIXED_ASPECT_RATIO_H

#include "Prereqs.h"
#include "Vector2D.h"

namespace Seoul::UI
{

namespace FixedAspectRatio
{

enum Enum
{
	kOff,

	kMotorolaRazr2019,
	kSonyXperia1,
	kSamsungGalaxyS20,
	kIphoneX,
	kIphoneXR,
	kSamsungGalaxyS10,
	kHuaweiP20,
	kHuaweiP20Pro,
	kSamsungGalaxyS9,
	kGooglePixel3,
	kLGK7,
	kIphone8,
	k9over16,
	kIphone5,
	kFuhuNabiJr,
	kNexus7_2013,
	k2over3,
	kIpadPro11,
	kIpadPro12point9,
	k3over4,
	k4over3,
	k3over2,
	k16over9,

	COUNT,
}; // enum Enum

extern Pair<Double, Double> const g_aDimensions[COUNT];

static inline Enum ToEnum(const Vector2D& v)
{
	// Zero is a special value for kOff.
	if (v != Vector2D::Zero())
	{
		auto const f = (Double)v.X / (Double)v.Y;

		for (Int i = 1 /* skip kOff */; i < COUNT; ++i)
		{
			auto const fCurrent = g_aDimensions[i].First / g_aDimensions[i].Second;
			if (Equals(f, fCurrent, 1e-4)) { return (Enum)i; }
		}
	}

	// Fall back to kOff.
	return kOff;
}

static inline void ToRatio(Enum e, Vector2D& rv)
{
	if (e < 1 /* kOff == 0 */ || e >= COUNT)
	{
		rv = Vector2D::Zero();
		return;
	}

	auto const pair = g_aDimensions[e];
	rv.X = (Float)pair.First;
	rv.Y = (Float)pair.Second;
}

} // namespace UI::FixedAspectRatio

} // namespace Seoul::UI

#endif // include guard
