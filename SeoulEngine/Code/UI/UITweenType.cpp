/**
 * \file UITweenType.cpp
 * \brief Describes the style of tweening applied to a UITween.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "UITweenType.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(UI::TweenType)
	SEOUL_ENUM_N("InOutCubic", UI::TweenType::kInOutCubic)
	SEOUL_ENUM_N("InOutQuadratic", UI::TweenType::kInOutQuadratic)
	SEOUL_ENUM_N("InOutQuartic", UI::TweenType::kInOutQuartic)
	SEOUL_ENUM_N("Line", UI::TweenType::kLine)
	SEOUL_ENUM_N("SinStartFast", UI::TweenType::kSinStartFast)
	SEOUL_ENUM_N("SinStartSlow", UI::TweenType::kSinStartSlow)
SEOUL_END_ENUM()

} // namespace Seoul
