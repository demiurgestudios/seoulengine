/**
 * \file ColorBlindViz.h
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

#pragma once
#ifndef COLOR_BLIND_VIZ_H
#define COLOR_BLIND_VIZ_H

#include "PostProcess.h"
#include "Prereqs.h"
#include "Singleton.h"

namespace Seoul
{

enum class ColorBlindVizMode
{
	kOff,

	/** "Red-green" color blindness, where the eye is less sensitive to green. */
	kDeutanopia,

	/** "Red-green" color blindness, where the eye is less sensitive to red. */
	kProtanopia,

	/** Color blindness where blue skews towards green and yellow skews toward violet. */
	kTritanopia,

	/** Complete color blindness - conversion to grayscale. */
	kAchromatopsia,

	MIN = kOff,
	MAX = kAchromatopsia,
};

/**
 * ColorBlindViz is a post-process that modifies viewport rendering to approximate
 * various types of color blindness.
 * 
 * ColorBlindViz is a developer tool for visualizing the approximate effects of
 * various color blindness and to facilitate adjusting art to account for color
 * blindness.
 */
class ColorBlindViz SEOUL_SEALED : public PostProcess
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ColorBlindViz);

	ColorBlindViz(const DataStoreTableUtil& configSettings);
	~ColorBlindViz();

	/** @return The current global color blind configuration. */
	static ColorBlindVizMode GetMode()
	{
		return s_eMode;
	}

	/** Update the current global color blind configuration. */
	static void SetMode(ColorBlindVizMode eMode)
	{
		s_eMode = eMode;
	}

private:
	static ColorBlindVizMode s_eMode;

	virtual HString GetEffectTechnique() const SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(ColorBlindViz);
}; // class ColorBlindViz

} // namespace Seoul

#endif // include guard
