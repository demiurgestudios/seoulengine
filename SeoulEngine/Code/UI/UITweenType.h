/**
 * \file UITweenType.h
 * \brief Describes the style of tweening applied to a UITween.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_TWEEN_TYPE_H
#define UI_TWEEN_TYPE_H

#include "Prereqs.h"

namespace Seoul::UI
{

enum class TweenType
{
	/** Ease in-out with a cubic shape. */
	kInOutCubic,

	/** Ease in-out with a quadratic shape. */
	kInOutQuadratic,

	/** Ease in-out with a cubic shape. */
	kInOutQuartic,

	/** Linear interpolation between start and end points - the default */
	kLine,

	/** Follows a sin() curve starting with a slope of 1, ending with slope 0 */
	kSinStartFast,

	/** Follows a sin() curve starting with a slope of 0, ending with slope 1 */
	kSinStartSlow,
};

} // namespace Seoul::UI

#endif // include guard
