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

#include "UIFixedAspectRatio.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(UI::FixedAspectRatio::Enum)
	SEOUL_ENUM_N("Off", UI::FixedAspectRatio::kOff)

	SEOUL_ENUM_N("0.408964: Motorola Razr 2019", UI::FixedAspectRatio::kMotorolaRazr2019)
	SEOUL_ENUM_N("0.428125: Sony Xperia 1", UI::FixedAspectRatio::kSonyXperia1)
	SEOUL_ENUM_N("0.450000: Samsung Galaxy S20", UI::FixedAspectRatio::kSamsungGalaxyS20)
	SEOUL_ENUM_N("0.461823: iPhone X", UI::FixedAspectRatio::kIphoneX)
	SEOUL_ENUM_N("0.462054: iPhone XR", UI::FixedAspectRatio::kIphoneXR)
	SEOUL_ENUM_N("0.473684: Samsung Galaxy S10", UI::FixedAspectRatio::kSamsungGalaxyS10)
	SEOUL_ENUM_N("0.481283: Huawei P20", UI::FixedAspectRatio::kHuaweiP20)
	SEOUL_ENUM_N("0.482143: Huawei P20 Pro", UI::FixedAspectRatio::kHuaweiP20Pro)
	SEOUL_ENUM_N("0.486486: Samsung Galaxy S9", UI::FixedAspectRatio::kSamsungGalaxyS9)
	SEOUL_ENUM_N("0.500000: Google Pixel 3", UI::FixedAspectRatio::kGooglePixel3)
	SEOUL_ENUM_N("0.562061: LG K7", UI::FixedAspectRatio::kLGK7)
	SEOUL_ENUM_N("0.562219: iPhone 8", UI::FixedAspectRatio::kIphone8)
	SEOUL_ENUM_N("0.562500: 9:16 Portrait", UI::FixedAspectRatio::k9over16)
	SEOUL_ENUM_N("0.563380: iPhone 5", UI::FixedAspectRatio::kIphone5)
	SEOUL_ENUM_N("0.600000: Fuhu Nabi Jr.", UI::FixedAspectRatio::kFuhuNabiJr)
	SEOUL_ENUM_N("0.625000: Nexus 7 (2013)", UI::FixedAspectRatio::kNexus7_2013)
	SEOUL_ENUM_N("0.666667: 2:3 Portrait", UI::FixedAspectRatio::k2over3)
	SEOUL_ENUM_N("0.698492: iPad Pro 11", UI::FixedAspectRatio::kIpadPro11)
	SEOUL_ENUM_N("0.749634: iPad Pro 12.9", UI::FixedAspectRatio::kIpadPro12point9)
	SEOUL_ENUM_N("0.750000: 3:4 Portrait", UI::FixedAspectRatio::k3over4)
	SEOUL_ENUM_N("1.333333: 4:3 Widescreen", UI::FixedAspectRatio::k4over3)
	SEOUL_ENUM_N("1.500000: 3:2 Widescreen", UI::FixedAspectRatio::k3over2)
	SEOUL_ENUM_N("1.777778: 16:9 Widescreen", UI::FixedAspectRatio::k16over9)

	// Aliases.
	SEOUL_ALIAS("9:16", "0.562500: 9:16 Portrait")
	SEOUL_ALIAS("2:3", "2:3 Portrait")
	SEOUL_ALIAS("3:4", "3:4 Portrait")
	SEOUL_ALIAS("4:3", "4:3 Portrait")
	SEOUL_ALIAS("3:2", "3:2 Portrait")
	SEOUL_ALIAS("16:9", "1.777778: 16:9 Widescreen")
SEOUL_END_ENUM()

namespace UI
{

namespace FixedAspectRatio
{

Pair<Double, Double> const g_aDimensions[COUNT] =
{
	MakePair(0, 0),      // kOff

	MakePair(876, 2142),  // kMotorolaRazr2019
	MakePair(1644, 3840), // kSonyXperia1
	MakePair(1440, 3200), // kSamsungGalaxyS20
	MakePair(1125, 2436), // kIphoneX
	MakePair(828, 1792),  // kIphoneXR
	MakePair(1440, 3040), // kSamsungGalaxyS10
	MakePair(1080, 2244), // kHuaweiP20
	MakePair(1080, 2240), // kHuaweiP20Pro
	MakePair(1440, 2960), // kSamsungGalaxyS9
	MakePair(1080, 2160), // kGooglePixel3
	MakePair(480, 854),   // kLGK7
	MakePair(750, 1334),  // kIphone8
	MakePair(1080, 1920), // k9over16
	MakePair(640, 1136),  // kIphone5
	MakePair(480, 800),   // kFuhuNabiJr
	MakePair(1200, 1920), // kNexus7_2013
	MakePair(640, 960),   // k2over3
	MakePair(1668, 2388), // kIpadPro11
	MakePair(2048, 2732), // kIpadPro12point9
	MakePair(1668, 2224), // k3over4
	MakePair(2224, 1668), // k4over3
	MakePair(960, 640),   // k3over2
	MakePair(1920, 1080), // k16over9
};

} // namespace UI::FixedAspectRatio

} // namespace UI

} // namespace Seoul
