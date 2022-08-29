/**
 * \file AnimationSlotBlendMode.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationSlotBlendMode.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_BEGIN_ENUM(Animation::SlotBlendMode)
	SEOUL_ENUM_N("None", Animation::SlotBlendMode::kNone)
	SEOUL_ENUM_N("OnlySource", Animation::SlotBlendMode::kOnlySource)
	SEOUL_ENUM_N("OnlyTarget", Animation::SlotBlendMode::kOnlyTarget)
SEOUL_END_ENUM()

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
