/**
 * \file ColorBlindViz.cpp
 * \brief PostProcess subclass (IPoseable) that applies a post-processing
 * step to visualize various approximated forms of
 * color blindness.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ColorBlindViz.h"
#include "ReflectionDefine.h"
#include "RenderPass.h"

namespace Seoul
{

static const HString kRenderDeutanopia("seoul_RenderDeutanopia");
static const HString kRenderProtanopia("seoul_RenderProtanopia");
static const HString kRenderTritanopia("seoul_RenderTritanopia");
static const HString kRenderAchromatopsia("seoul_RenderAchromatopsia");

SEOUL_BEGIN_ENUM(ColorBlindVizMode)
	SEOUL_ENUM_N("Off", ColorBlindVizMode::kOff)
	SEOUL_ENUM_N("Deutanopia", ColorBlindVizMode::kDeutanopia)
	SEOUL_ENUM_N("Protanopia", ColorBlindVizMode::kProtanopia)
	SEOUL_ENUM_N("Tritanopia", ColorBlindVizMode::kTritanopia)
	SEOUL_ENUM_N("Achromatopsia", ColorBlindVizMode::kAchromatopsia)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(ColorBlindViz, TypeFlags::kDisableNew)
	SEOUL_PARENT(PostProcess)
SEOUL_END_TYPE()

ColorBlindVizMode ColorBlindViz::s_eMode(ColorBlindVizMode::kOff);

ColorBlindViz::ColorBlindViz(const DataStoreTableUtil& configSettings)
	: PostProcess(configSettings)
{
}

ColorBlindViz::~ColorBlindViz()
{
}

/** @return The effect technique to use for the current color blind visualization mode. */
HString ColorBlindViz::GetEffectTechnique() const
{
	switch (s_eMode)
	{
	case ColorBlindVizMode::kDeutanopia: return kRenderDeutanopia;
	case ColorBlindVizMode::kProtanopia: return kRenderProtanopia;
	case ColorBlindVizMode::kTritanopia: return kRenderTritanopia;
	case ColorBlindVizMode::kAchromatopsia: return kRenderAchromatopsia;
	default:
		return kRenderDeutanopia;
	};
}

} // namespace Seoul
