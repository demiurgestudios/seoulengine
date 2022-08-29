/**
 * \file UITweenTarget.cpp
 * \brief Enum describes the property that will be manipulated
 * by a UITween.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "UITweenTarget.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(UI::TweenTarget)
	SEOUL_ENUM_N("Alpha", UI::TweenTarget::kAlpha)
	SEOUL_ENUM_N("Depth3D", UI::TweenTarget::kDepth3D)
	SEOUL_ENUM_N("PositionX", UI::TweenTarget::kPositionX)
	SEOUL_ENUM_N("PositionY", UI::TweenTarget::kPositionY)
	SEOUL_ENUM_N("Rotation", UI::TweenTarget::kRotation)
	SEOUL_ENUM_N("ScaleX", UI::TweenTarget::kScaleX)
	SEOUL_ENUM_N("ScaleY", UI::TweenTarget::kScaleY)
	SEOUL_ENUM_N("Timer", UI::TweenTarget::kTimer)
SEOUL_END_ENUM()

} // namespace Seoul
